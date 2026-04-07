#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <functional>

//==============================================================================
// LicenseValidator — Validates Dawmuze licenses against the license server
//
// Flow:
//   1. On first launch: user enters license key → activate()
//   2. Key + machine fingerprint sent to server → activation stored
//   3. Periodic validation (every 24h) via validate()
//   4. If offline: grace period of 7 days before lockout
//   5. License data cached locally with XOR obfuscation (not encryption —
//      see PACATA audit notes; acceptable for local cache anti-tampering)
//
// Integration:
//   DAWMainComponent checks isLicensed() on startup.
//   If false → shows activation dialog.
//   If trial → shows trial banner with days remaining.
//==============================================================================

class LicenseValidator : public juce::Timer
{
public:
    enum class Status
    {
        NotActivated,
        Active,
        Trial,
        Expired,
        Revoked,
        OfflineGrace,   // Valid but can't reach server (within grace period)
        Locked          // Grace period exceeded
    };

    LicenseValidator()
    {
        loadCachedLicense();
    }

    ~LicenseValidator() override
    {
        // Invalidate alive flag FIRST — prevents pending callAsync lambdas
        // from accessing this object after destruction.
        aliveFlag_->store(false);
        stopTimer();
    }

    //--------------------------------------------------------------------------
    // Status queries
    //--------------------------------------------------------------------------
    Status getStatus() const { return status_; }
    bool isLicensed() const { return status_ == Status::Active || status_ == Status::OfflineGrace; }
    bool isTrial() const { return status_ == Status::Trial; }
    int getTrialDaysRemaining() const { return trialDaysRemaining_; }
    juce::String getLicenseKey() const { return licenseKey_; }
    juce::String getStatusString() const
    {
        switch (status_)
        {
            case Status::NotActivated: return "Not Activated";
            case Status::Active:       return "Active";
            case Status::Trial:        return "Trial (" + juce::String(trialDaysRemaining_) + " days left)";
            case Status::Expired:      return "Expired";
            case Status::Revoked:      return "Revoked";
            case Status::OfflineGrace: return "Active (Offline)";
            case Status::Locked:       return "Locked";
        }
        return "Unknown";
    }

    //--------------------------------------------------------------------------
    // Set server URL — only HTTPS URLs are accepted for non-local hosts
    // to prevent downgrade attacks. Localhost/127.0.0.1 are allowed over
    // HTTP for local development.
    //--------------------------------------------------------------------------
    void setServerUrl(const juce::String& url)
    {
    #if JUCE_DEBUG
        if (!url.startsWith("https://") && !url.contains("localhost") && !url.contains("127.0.0.1"))
            return; // reject non-HTTPS for non-local URLs
    #else
        if (!url.startsWith("https://"))
            return; // reject all non-HTTPS in production builds
    #endif
        serverUrl_ = url;
    }

    //--------------------------------------------------------------------------
    // Generate machine fingerprint
    //--------------------------------------------------------------------------
    juce::String getMachineId() const
    {
        // Combine multiple system identifiers for a stable fingerprint
        auto ids = juce::SystemStats::getUniqueDeviceID();
        auto computerName = juce::SystemStats::getComputerName();
        auto userName = juce::SystemStats::getLogonName();

        juce::String combined = ids + "|" + computerName + "|" + userName;
        auto hash = juce::SHA256(combined.toUTF8(), combined.getNumBytesAsUTF8());
        return hash.toHexString().substring(0, 32);
    }

    //--------------------------------------------------------------------------
    // Activate license key
    //--------------------------------------------------------------------------
    void activate(const juce::String& key, std::function<void(bool success, juce::String message)> callback)
    {
        licenseKey_ = key.trim().toUpperCase();

        auto machineId = getMachineId();
        auto machineName = juce::SystemStats::getComputerName();
        auto osInfo = juce::SystemStats::getOperatingSystemName();

        // Build JSON payload
        auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
        json->setProperty("licenseKey", licenseKey_);
        json->setProperty("machineId", machineId);
        json->setProperty("machineName", machineName);
        json->setProperty("osInfo", osInfo);

        auto payload = juce::JSON::toString(juce::var(json.get()));
        auto url = juce::URL(serverUrl_ + "/api/licenses/activate")
                       .withPOSTData(payload);

        // Run on background thread — capture aliveFlag_ (shared_ptr) to guard
        // against use-after-free if this object is destroyed before callAsync fires.
        auto alive = aliveFlag_;
        juce::Thread::launch([this, url, callback, alive]()
        {
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                               .withExtraHeaders("Content-Type: application/json")
                               .withConnectionTimeoutMs(10000);

            auto stream = url.createInputStream(options);

            if (stream == nullptr)
            {
                juce::MessageManager::callAsync([callback]()
                {
                    callback(false, "Could not connect to license server");
                });
                return;
            }

            auto response = stream->readEntireStreamAsString();
            auto parsed = juce::JSON::parse(response);

            auto status = parsed.getProperty("status", "").toString();
            auto message = parsed.getProperty("message", "").toString();
            auto error = parsed.getProperty("error", "").toString();

            bool success = (status == "active");

            if (success)
            {
                juce::MessageManager::callAsync([this, callback, message, alive]()
                {
                    if (!alive->load())
                        return;  // Object was destroyed — abort

                    status_ = Status::Active;
                    lastValidation_ = juce::Time::getCurrentTime();
                    saveCachedLicense();
                    startTimer(30 * 1000); // Heartbeat every 30 sec // Validate every 24h
                    callback(true, message.isEmpty() ? "License activated!" : message);
                });
            }
            else
            {
                juce::MessageManager::callAsync([callback, error]()
                {
                    callback(false, error.isEmpty() ? "Activation failed" : error);
                });
            }
        });
    }

    //--------------------------------------------------------------------------
    // Periodic validation (called by timer and on startup)
    //--------------------------------------------------------------------------
    void validate()
    {
        if (licenseKey_.isEmpty())
            return;

        auto machineId = getMachineId();

        auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
        json->setProperty("licenseKey", licenseKey_);
        json->setProperty("machineId", machineId);

        auto payload = juce::JSON::toString(juce::var(json.get()));
        auto url = juce::URL(serverUrl_ + "/api/licenses/validate")
                       .withPOSTData(payload);

        // Capture aliveFlag_ (shared_ptr) to guard against use-after-free
        // if this object is destroyed before callAsync fires.
        auto alive = aliveFlag_;
        juce::Thread::launch([this, url, alive]()
        {
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                               .withExtraHeaders("Content-Type: application/json")
                               .withConnectionTimeoutMs(10000);

            auto stream = url.createInputStream(options);

            if (stream == nullptr)
            {
                // Offline — check grace period
                juce::MessageManager::callAsync([this, alive]()
                {
                    if (!alive->load())
                        return;  // Object was destroyed — abort

                    auto now = juce::Time::getCurrentTime();
                    auto daysSinceValidation = (now - lastValidation_).inDays();

                    // Anti-clock-manipulation: detect backward clock
                    if (daysSinceValidation < 0)
                    {
                        status_ = Status::Locked;
                    }
                    // Anti-clock-manipulation: if we have a cached server time,
                    // check if local clock diverges too far from expected progression
                    else if (lastServerTime_ > 0)
                    {
                        auto localEpoch = now.toMilliseconds() / 1000;
                        auto expectedNow = lastServerTime_ + static_cast<int64_t>(daysSinceValidation * 86400);
                        auto drift = std::abs(localEpoch - expectedNow);
                        if (drift > 72 * 3600) // > 72h drift while offline = suspicious
                            status_ = Status::Locked;
                        else if (daysSinceValidation <= 7)
                            status_ = Status::OfflineGrace;
                        else
                            status_ = Status::Locked;
                    }
                    else if (daysSinceValidation <= 7)
                    {
                        status_ = Status::OfflineGrace;
                    }
                    else
                        status_ = Status::Locked;

                    if (onStatusChanged)
                        onStatusChanged(status_);
                });
                return;
            }

            auto response = stream->readEntireStreamAsString();
            auto parsed = juce::JSON::parse(response);

            bool valid = static_cast<bool>(parsed.getProperty("valid", false));
            auto reason = parsed.getProperty("reason", "").toString();

            // Server-signed timestamp for clock drift detection
            int64_t serverTime = static_cast<int64_t>(parsed.getProperty("serverTime", (int64_t)0));

            juce::MessageManager::callAsync([this, valid, reason, alive, serverTime]()
            {
                if (!alive->load())
                    return;  // Object was destroyed — abort

                if (valid)
                {
                    // Clock manipulation check: if server time diverges from local by > 72h, lock
                    // Using 72h threshold to account for laptops sleeping, timezone issues,
                    // and NTP sync delays. This is generous enough to avoid false positives
                    // but catches intentional clock manipulation.
                    if (serverTime > 0)
                    {
                        auto localEpoch = juce::Time::getCurrentTime().toMilliseconds() / 1000;
                        auto drift = std::abs(localEpoch - serverTime);
                        if (drift > 72 * 3600) // > 72 hours drift = suspicious
                        {
                            status_ = Status::Locked;
                            saveCachedLicense();
                            if (onStatusChanged)
                                onStatusChanged(status_);
                            return;
                        }
                        lastServerTime_ = serverTime;
                    }

                    status_ = Status::Active;
                    lastValidation_ = juce::Time::getCurrentTime();
                    saveCachedLicense();
                }
                else
                {
                    if (reason == "expired") status_ = Status::Expired;
                    else if (reason == "revoked") status_ = Status::Revoked;
                    else status_ = Status::Locked;

                    // Clear cached license if revoked
                    if (status_ == Status::Revoked)
                        clearCachedLicense();
                }

                if (onStatusChanged)
                    onStatusChanged(status_);
            });
        });
    }

    //--------------------------------------------------------------------------
    // Trial management
    //--------------------------------------------------------------------------
    // NOTE: Trial abuse prevention requires server-side trial tracking.
    // The local cache alone is insufficient — users can delete
    // ~/Library/Application Support/Dawmuze/.license to reset trial.
    // TODO: Add POST /api/trial/start endpoint that records (machineId, firstSeen)
    // and returns the actual trial start date. The client should call that
    // endpoint before granting trial access, falling back to local cache
    // only when offline (with a shorter offline trial grace of 1 day).
    void startTrial()
    {
        if (trialStartDate_ == juce::Time())
        {
            trialStartDate_ = juce::Time::getCurrentTime();
            status_ = Status::Trial;
            trialDaysRemaining_ = 14;
            saveCachedLicense();
        }
    }

    void checkTrial()
    {
        if (trialStartDate_ != juce::Time())
        {
            int elapsed = static_cast<int>((juce::Time::getCurrentTime() - trialStartDate_).inDays());
            trialDaysRemaining_ = juce::jmax(0, 14 - elapsed);

            if (trialDaysRemaining_ <= 0)
            {
                status_ = Status::Expired;
                trialDaysRemaining_ = 0;
            }
            else
            {
                status_ = Status::Trial;
            }
        }
    }

    //--------------------------------------------------------------------------
    // Deactivate (free up a machine slot)
    //--------------------------------------------------------------------------
    void deactivate()
    {
        clearCachedLicense();
        status_ = Status::NotActivated;
        licenseKey_.clear();
        stopTimer();
    }

    //--------------------------------------------------------------------------
    // Callback: status changed (for UI updates)
    //--------------------------------------------------------------------------
    std::function<void(Status)> onStatusChanged;

    //--------------------------------------------------------------------------
    // Timer callback — heartbeat every 5 min, full validate every 24h
    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        heartbeatCount_++;
        // Full validation every 24h (2880 x 30sec = 24h)
        if (heartbeatCount_ >= 2880)
        {
            heartbeatCount_ = 0;
            validate();
        }
        else if (status_ == Status::Active)
        {
            // Lightweight heartbeat — just update last_seen on server
            sendHeartbeat();
        }

        // If in Trial/Expired/NotActivated, check server for new license every 60s
        if (status_ != Status::Active && heartbeatCount_ % 2 == 0)
            checkServerLicense();
    }

    void setHostName(const juce::String& name) { hostName_ = name; }

    // Check server for license using Hub session token
    // Called periodically — if user got a license (admin gave it), plugin auto-activates
    void checkServerLicense()
    {
        // Read Hub session file for user token
        auto pluginUserFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Dawmuze").getChildFile(".plugin-user");
        if (!pluginUserFile.existsAsFile()) return;

        auto content = pluginUserFile.loadFileAsString();
        auto parsed = juce::JSON::parse(content);
        if (parsed.isVoid()) return;

        auto token = parsed.getProperty("token", "").toString();
        if (token.isEmpty()) return;

        auto alive = aliveFlag_;
        auto serverUrl = serverUrl_;

        juce::Thread::launch([this, token, serverUrl, alive]()
        {
            // Check if user has an active license
            auto url = juce::URL(serverUrl + "/api/licenses");
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                               .withExtraHeaders("Authorization: Bearer " + token)
                               .withConnectionTimeoutMs(10000);
            auto stream = url.createInputStream(options);
            if (!stream) return;

            auto response = stream->readEntireStreamAsString();
            auto data = juce::JSON::parse(response);
            if (data.isVoid()) return;

            auto* licenses = data.getProperty("licenses", juce::var()).getArray();
            if (licenses == nullptr || licenses->isEmpty()) return;

            // Look for an active dawmuze-ki2a license
            for (auto& lic : *licenses)
            {
                auto product = lic.getProperty("product", "").toString();
                auto licStatus = lic.getProperty("status", "").toString();
                auto key = lic.getProperty("license_key", "").toString();

                if (product == "dawmuze-ki2a" && licStatus == "active" && key.isNotEmpty())
                {
                    // Found a license! Auto-activate
                    juce::MessageManager::callAsync([this, key, alive]()
                    {
                        if (!alive->load()) return;
                        if (status_ == Status::Active) return; // Already active

                        licenseKey_ = key;
                        status_ = Status::Active;
                        lastValidation_ = juce::Time::getCurrentTime();
                        saveCachedLicense();

                        if (onStatusChanged)
                            onStatusChanged(status_);
                    });
                    return;
                }
            }
        });
    }

    void sendHeartbeat()
    {
        if (licenseKey_.isEmpty() || status_ != Status::Active)
            return;

        auto machineId = getMachineId();
        auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
        json->setProperty("licenseKey", licenseKey_);
        json->setProperty("machineId", machineId);
        if (hostName_.isNotEmpty())
            json->setProperty("daw", hostName_);

        auto payload = juce::JSON::toString(juce::var(json.get()));
        auto url = juce::URL(serverUrl_ + "/api/licenses/heartbeat")
                       .withPOSTData(payload);

        // Fire-and-forget on background thread — no UI callback needed
        juce::Thread::launch([url]()
        {
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                               .withExtraHeaders("Content-Type: application/json")
                               .withConnectionTimeoutMs(5000);
            url.createInputStream(options); // fire and forget
        });
    }

private:
    //--------------------------------------------------------------------------
    // Local license cache (encrypted with machine-specific key)
    //--------------------------------------------------------------------------
    juce::File getCacheFile() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Dawmuze")
                   .getChildFile(".license");
    }

    void saveCachedLicense()
    {
        auto file = getCacheFile();
        file.getParentDirectory().createDirectory();

        auto json = juce::DynamicObject::Ptr(new juce::DynamicObject());
        json->setProperty("key", licenseKey_);
        json->setProperty("status", static_cast<int>(status_));
        json->setProperty("lastValidation", lastValidation_.toISO8601(true));
        json->setProperty("machineId", getMachineId());

        if (trialStartDate_ != juce::Time())
            json->setProperty("trialStart", trialStartDate_.toISO8601(true));

        if (lastServerTime_ > 0)
            json->setProperty("serverTime", lastServerTime_);

        auto data = juce::JSON::toString(juce::var(json.get()));

        // NOTE: This XOR is intentional *obfuscation only* — it prevents casual
        // users from reading/editing the cached license JSON in a text editor.
        // It is NOT cryptographic security. The actual license validation is
        // performed server-side; this local cache merely avoids re-activation
        // on every launch and supports the 7-day offline grace period.
        // (PACATA audit acknowledgement: weak XOR cipher is acceptable here
        // because the threat model is casual tampering, not a determined attacker.)
        auto machineKey = getMachineId();
        int keyLen = machineKey.length();
        juce::MemoryBlock block(data.toUTF8(), static_cast<size_t>(data.getNumBytesAsUTF8()));
        if (keyLen > 0)
        {
            for (size_t i = 0; i < block.getSize(); ++i)
                static_cast<uint8_t*>(block.getData())[i] ^=
                    static_cast<uint8_t>(machineKey[static_cast<int>(i % static_cast<size_t>(keyLen))]);
        }

        file.replaceWithData(block.getData(), block.getSize());
    }

    void loadCachedLicense()
    {
        auto file = getCacheFile();
        if (!file.existsAsFile())
        {
            status_ = Status::NotActivated;
            return;
        }

        juce::MemoryBlock block;
        file.loadFileAsData(block);

        // De-obfuscate (XOR with machine ID — see saveCachedLicense() comment)
        auto machineKey = getMachineId();
        int keyLen = machineKey.length();
        if (keyLen > 0)
        {
            for (size_t i = 0; i < block.getSize(); ++i)
                static_cast<uint8_t*>(block.getData())[i] ^=
                    static_cast<uint8_t>(machineKey[static_cast<int>(i % static_cast<size_t>(keyLen))]);
        }

        auto data = juce::String::fromUTF8(static_cast<const char*>(block.getData()),
                                            static_cast<int>(block.getSize()));
        auto parsed = juce::JSON::parse(data);

        if (parsed.isVoid())
        {
            status_ = Status::NotActivated;
            return;
        }

        // Verify machine ID matches
        auto storedMachine = parsed.getProperty("machineId", "").toString();
        if (storedMachine != getMachineId())
        {
            status_ = Status::NotActivated;
            return;
        }

        licenseKey_ = parsed.getProperty("key", "").toString();
        int statusInt = static_cast<int>(parsed.getProperty("status", 0));
        status_ = static_cast<Status>(juce::jlimit(0, static_cast<int>(Status::Locked), statusInt));

        auto validStr = parsed.getProperty("lastValidation", "").toString();
        if (validStr.isNotEmpty())
            lastValidation_ = juce::Time::fromISO8601(validStr);

        auto trialStr = parsed.getProperty("trialStart", "").toString();
        if (trialStr.isNotEmpty())
        {
            trialStartDate_ = juce::Time::fromISO8601(trialStr);
            checkTrial();
        }

        lastServerTime_ = static_cast<int64_t>(parsed.getProperty("serverTime", (int64_t)0));

        // If previously active, start validation timer and validate now
        if (status_ == Status::Active || status_ == Status::OfflineGrace)
        {
            startTimer(30 * 1000); // Heartbeat every 30 sec
            validate();
        }
    }

    void clearCachedLicense()
    {
        auto file = getCacheFile();
        if (file.existsAsFile())
            file.deleteFile();
    }

    //--------------------------------------------------------------------------
    // Members
    //--------------------------------------------------------------------------
    juce::String serverUrl_ = "https://dawmuze.com";
    juce::String licenseKey_;
    Status status_ = Status::NotActivated;
    juce::Time lastValidation_;
    juce::Time trialStartDate_;
    int trialDaysRemaining_ = 0;
    int heartbeatCount_ = 0;      // Counts 30-sec intervals; resets at 2880 (24h) for full validate
    juce::String hostName_;        // DAW/host name for analytics
    int64_t lastServerTime_ = 0;  // Server-signed epoch for clock drift detection

    // Alive flag for guarding callAsync lambdas (shared_ptr survives object destruction)
    std::shared_ptr<std::atomic<bool>> aliveFlag_ = std::make_shared<std::atomic<bool>>(true);
};

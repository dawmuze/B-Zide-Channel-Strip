#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

//==============================================================================
// BZideGate — Gate/Expander for GATE section
//==============================================================================
class BZideGate
{
public:
    enum Mode { GATE = 0, EXPANDER };

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr_ = sampleRate;
        envelope_ = 0.0f;
        gateOpen_ = false;
        smoothedGateGain_ = 1.0f;

        // SC bandpass filters: HPF @ 100Hz + LPF @ 8kHz
        auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 100.0f);
        auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 8000.0f);
        scHpfL_.coefficients = hpCoeffs;
        scHpfR_.coefficients = hpCoeffs;
        scLpfL_.coefficients = lpCoeffs;
        scLpfR_.coefficients = lpCoeffs;
        scHpfL_.reset(); scHpfR_.reset();
        scLpfL_.reset(); scLpfR_.reset();
    }

    void setMode(Mode m) { mode_ = m; }
    void setThreshold(float db) { threshold_ = db; }
    void setAttenuation(float db) { atten_ = db; }
    void setFloor(float db) { floor_ = db; }
    void setAttack(float ms) { attackMs_ = juce::jmax(0.1f, ms); }
    void setRelease(float ms) { releaseMs_ = juce::jmax(10.0f, ms); }
    void setBypass(bool b) { bypass_ = b; }
    void setFast(bool f) { fast_ = f; }
    void setPeakDetect(bool p) { peakDetect_ = p; }
    void setScEnabled(bool e) { scEnabled_ = e; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_) return;

        // Fast mode overrides: attack = 0.1ms, release /= 4
        float effectiveAttack = fast_ ? 0.1f : attackMs_;
        float effectiveRelease = fast_ ? (releaseMs_ / 4.0f) : releaseMs_;

        float attackCoeff = std::exp(-1.0f / (float)(sr_ * effectiveAttack * 0.001));
        float releaseCoeff = std::exp(-1.0f / (float)(sr_ * juce::jmax(10.0f, effectiveRelease) * 0.001));
        float threshLin = juce::Decibels::decibelsToGain(threshold_);
        float floorLin = juce::Decibels::decibelsToGain(floor_);
        float peakGR = 0.0f;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Detection signal
            float detL = buffer.getSample(0, i);
            float detR = buffer.getNumChannels() > 1 ? buffer.getSample(1, i) : detL;

            // SC bandpass (100Hz-8kHz) on detection signal
            if (scEnabled_)
            {
                detL = scLpfL_.processSample(scHpfL_.processSample(detL));
                detR = scLpfR_.processSample(scHpfR_.processSample(detR));
            }

            float level = 0.0f;
            if (peakDetect_)
            {
                // Peak detection
                level = std::max(std::abs(detL), std::abs(detR));
            }
            else
            {
                // RMS detection
                float sq = std::max(detL * detL, detR * detR);
                level = std::sqrt(sq);
            }

            // Envelope
            if (level > envelope_)
                envelope_ = attackCoeff * envelope_ + (1.0f - attackCoeff) * level;
            else
                envelope_ = releaseCoeff * envelope_ + (1.0f - releaseCoeff) * level;

            // Phase 2A: Gate hysteresis (~4dB between open/close)
            float openThresh = threshLin;
            float closeThresh = threshLin * 0.63f; // ~4dB below open threshold
            if (!gateOpen_ && envelope_ > openThresh) gateOpen_ = true;
            if (gateOpen_ && envelope_ < closeThresh) gateOpen_ = false;

            // Gate gain based on hysteresis state
            float attenGain = floorLin;
            if (mode_ == EXPANDER && !gateOpen_)
                attenGain = std::max(floorLin, envelope_ / threshLin);

            float targetGain = gateOpen_ ? 1.0f : attenGain;

            // Phase 2B: Gain smoothing (~1ms, SR-scaled)
            float gateSmooth = std::exp(-1.0f / (float)(sr_ * 0.001));
            smoothedGateGain_ = gateSmooth * smoothedGateGain_ + (1.0f - gateSmooth) * targetGain;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * smoothedGateGain_);

            // Track GR for meter display
            float grDb = -juce::Decibels::gainToDecibels(smoothedGateGain_);
            if (grDb > peakGR) peakGR = grDb;
        }

        gainReduction_ = peakGR;
    }

    float getGainReduction() const { return gainReduction_; }

private:
    double sr_ = 44100.0;
    Mode mode_ = GATE;
    float threshold_ = -40.0f;
    float atten_ = -30.0f;
    float floor_ = -60.0f;
    float attackMs_ = 1.0f;
    float releaseMs_ = 200.0f;
    bool bypass_ = true;
    bool fast_ = false;
    bool peakDetect_ = true;
    bool scEnabled_ = false;
    float envelope_ = 0.0f;
    bool gateOpen_ = false;            // Phase 2A: hysteresis state
    float smoothedGateGain_ = 1.0f;    // Phase 2B: gain smoothing
    float gainReduction_ = 0.0f;       // Peak GR per block for meter display

    // Sidechain bandpass filters (HPF + LPF per channel)
    juce::dsp::IIR::Filter<float> scHpfL_, scHpfR_;
    juce::dsp::IIR::Filter<float> scLpfL_, scLpfR_;
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/BZideSaturation.h"
#include "DSP/BZideCompressor.h"
#include "DSP/BZideGate.h"
#include "DSP/BZideLimiter.h"
#include "DSP/DeEsserProcessor.h"
#include "DSP/LA2ACompressor.h"
#include "LicenseValidator.h"

//==============================================================================
// InsertProcessor — wraps any internal DSP module for use in an insert slot
//==============================================================================
class InsertProcessor
{
public:
    enum ModuleType { None = 0, Saturation = 1, Compressor = 2, Gate = 3,
                      DeEsser = 4, Limiter = 5, LA2A = 6 };

    InsertProcessor() = default;
    InsertProcessor(InsertProcessor&&) = default;
    InsertProcessor& operator=(InsertProcessor&&) = default;

    void loadModule(ModuleType type, double sampleRate, int samplesPerBlock)
    {
        moduleType_ = type;
        bypassed_ = false;

        switch (type)
        {
            case Saturation:
                saturation_ = std::make_unique<BZideSaturation>();
                saturation_->prepare(sampleRate, samplesPerBlock);
                saturation_->setBypass(false);
                saturation_->setDrive(30.0f);
                saturation_->setTone(0.0f);
                saturation_->setMode(BZideSaturation::ODD);
                break;
            case Compressor:
                compressor_ = std::make_unique<BZideCompressor>();
                compressor_->prepare(sampleRate, samplesPerBlock);
                compressor_->setBypass(false);
                compressor_->setThreshold(-20.0f);
                compressor_->setRatio(4.0f);
                compressor_->setAttack(10.0f);
                compressor_->setRelease(100.0f);
                compressor_->setMakeupGain(0.0f);
                compressor_->setMix(100.0f);
                break;
            case Gate:
                gate_ = std::make_unique<BZideGate>();
                gate_->prepare(sampleRate, samplesPerBlock);
                gate_->setBypass(false);
                gate_->setThreshold(-40.0f);
                gate_->setAttenuation(-30.0f);
                gate_->setFloor(-60.0f);
                gate_->setAttack(1.0f);
                gate_->setRelease(200.0f);
                break;
            case DeEsser:
                deEsser_ = std::make_unique<DeEsserProcessor>();
                deEsser_->prepare(sampleRate, samplesPerBlock);
                deEsser_->setBand(0, 6000.0f, -20.0f, -12.0f);
                deEsser_->setBand(1, 10000.0f, -20.0f, -12.0f);
                break;
            case Limiter:
                limiter_ = std::make_unique<BZideLimiter>();
                limiter_->prepare(sampleRate, samplesPerBlock);
                limiter_->setBypass(false);
                limiter_->setThreshold(-0.3f);
                break;
            case LA2A:
                la2a_ = std::make_unique<LA2ACompressor>();
                la2a_->prepare(sampleRate, samplesPerBlock);
                la2a_->setPeakReduction(0.5f);
                la2a_->setGain(0.5f);
                break;
            default:
                break;
        }
    }

    void unload()
    {
        moduleType_ = None;
        saturation_.reset();
        compressor_.reset();
        gate_.reset();
        deEsser_.reset();
        limiter_.reset();
        la2a_.reset();
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (moduleType_ == None || bypassed_) return;

        switch (moduleType_)
        {
            case Saturation: if (saturation_) saturation_->process(buffer); break;
            case Compressor: if (compressor_) compressor_->process(buffer); break;
            case Gate:       if (gate_)       gate_->process(buffer);       break;
            case DeEsser:    if (deEsser_)    deEsser_->process(buffer);    break;
            case Limiter:    if (limiter_)     limiter_->process(buffer);    break;
            case LA2A:       if (la2a_)        la2a_->process(buffer);       break;
            default: break;
        }
    }

    void setBypass(bool b) { bypassed_ = b; }
    bool isBypassed() const { return bypassed_; }
    ModuleType getModuleType() const { return moduleType_; }
    bool isLoaded() const { return moduleType_ != None; }

    static juce::String getModuleName(ModuleType type)
    {
        switch (type)
        {
            case Saturation: return "Saturation";
            case Compressor: return "Compressor";
            case Gate:       return "Gate";
            case DeEsser:    return "De-Esser";
            case Limiter:    return "Limiter";
            case LA2A:       return "LA-2A";
            default:         return "---";
        }
    }

private:
    ModuleType moduleType_ = None;
    bool bypassed_ = false;

    std::unique_ptr<BZideSaturation> saturation_;
    std::unique_ptr<BZideCompressor> compressor_;
    std::unique_ptr<BZideGate> gate_;
    std::unique_ptr<DeEsserProcessor> deEsser_;
    std::unique_ptr<BZideLimiter> limiter_;
    std::unique_ptr<LA2ACompressor> la2a_;
};

//==============================================================================
class BZideProcessor : public juce::AudioProcessor
{
public:
    BZideProcessor();
    ~BZideProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "B-Zide Channel Strip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    LicenseValidator& getLicenseValidator() { return licenseValidator_; }

    // Metering (atomic for thread safety)
    std::atomic<float> inputLevelL { -100.0f }, inputLevelR { -100.0f };
    std::atomic<float> outputLevelL { -100.0f }, outputLevelR { -100.0f };
    std::atomic<float> gainReduction { 0.0f };

    // FFT data for spectrum analyzer
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder; // 2048
    float fftData[2 * fftSize] = {};
    bool nextFFTBlockReady = false;
    std::atomic<int> fifoIndex{0};
    float fifo[fftSize] = {};
    void pushNextSampleIntoFifo(float sample);

    // License
    bool isLicensed() const { return licenseValidator_.isLicensed(); }
    bool isTrial() const { return licenseValidator_.isTrial(); }
    bool isTrialExpired() const { return licenseValidator_.getStatus() == LicenseValidator::Status::Expired; }
    LicenseValidator::Status getLicenseStatus() const { return licenseValidator_.getStatus(); }
    int getTrialDaysRemaining() const { return licenseValidator_.getTrialDaysRemaining(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float getLimiterGR() const { return limiter_.getGainReduction(); }

    // Section names for parameters
    static juce::String getPreParamId(const juce::String& param) { return "pre_" + param; }
    static juce::String getEqParamId(const juce::String& param) { return "eq_" + param; }
    static juce::String getDsParamId(const juce::String& param) { return "ds_" + param; }
    static juce::String getCompParamId(const juce::String& param) { return "comp_" + param; }
    static juce::String getGateParamId(const juce::String& param) { return "gate_" + param; }
    static juce::String getOutParamId(const juce::String& param) { return "out_" + param; }

    // Section processing order (for drag-to-reorder)
    // Values: 0=PRE, 1=EQ, 2=DS2, 3=COMP, 4=GATE
    std::atomic<int> sectionOrder_[6] = { 0, 1, 2, 3, 4, 5 };
    void setSectionOrder(const std::array<int, 6>& order);

    // Individual DSP processing methods
    void processPre(juce::AudioBuffer<float>& buffer);
    void processEQ(juce::AudioBuffer<float>& buffer);
    void processDS2(juce::AudioBuffer<float>& buffer);
    void processComp(juce::AudioBuffer<float>& buffer);
    void processGate(juce::AudioBuffer<float>& buffer);

    // ── Insert slots (10 slots for internal DSP modules) ──
    static constexpr int numInsertSlots = 10;
    InsertProcessor insertSlots_[numInsertSlots];

    void loadInsert(int slotIndex, InsertProcessor::ModuleType type);
    void removeInsert(int slotIndex);
    void setInsertBypass(int slotIndex, bool bypassed);
    void swapInserts(int indexA, int indexB);
    void processInserts(juce::AudioBuffer<float>& buffer);

private:
    juce::AudioProcessorValueTreeState apvts;
    LicenseValidator licenseValidator_;

    // DSP processors (standalone — no apvts dependency)
    BZideSaturation saturation_;
    DeEsserProcessor deEsser_;
    BZideCompressor compressor_;
    BZideGate gate_;
    BZideLimiter limiter_;

    // EQ filters (3-band + HPF + LPF)
    using MonoChain = juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,  // HPF
        juce::dsp::IIR::Filter<float>,  // Low shelf
        juce::dsp::IIR::Filter<float>,  // Mid peak
        juce::dsp::IIR::Filter<float>,  // High shelf
        juce::dsp::IIR::Filter<float>   // LPF
    >;
    MonoChain leftEQ_, rightEQ_;

    double currentSampleRate_ = 44100.0;

    // LOWRIDE sub bass boost filters
    juce::dsp::IIR::Filter<float> lowrideL_, lowrideR_;

    // Pre-section LPF filters
    juce::dsp::IIR::Filter<float> preLpfL_, preLpfR_;

    void updateEQ();

    // Pre-allocated dry buffer for processPre() mix (FIX 2: avoid per-block allocation)
    juce::AudioBuffer<float> preDryBuffer_;

    // Cached EQ params to avoid per-block coefficient allocation (FIX 1)
    float cachedHpf_ = -1, cachedLpf_ = -1;
    float cachedLowF_ = -1, cachedLowG_ = -1, cachedLowQ_ = -1;
    int cachedLowType_ = -1;
    float cachedMidF_ = -1, cachedMidG_ = -1, cachedMidQ_ = -1;
    int cachedMidType_ = -1;
    float cachedHiF_ = -1, cachedHiG_ = -1, cachedHiQ_ = -1;
    int cachedHiType_ = -1;
    int cachedHpfSlope_ = -1, cachedLpfSlope_ = -1;

    // Cached LOWRIDE params (FIX 4)
    float cachedLowrideBoost_ = -1;

    // Cached Pre-LPF params (FIX 5)
    float cachedPreLpf_ = -1;

    // Pending insert swap for thread safety (FIX 8)
    std::atomic<int> pendingSwapA_{-1}, pendingSwapB_{-1};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BZideProcessor)
};

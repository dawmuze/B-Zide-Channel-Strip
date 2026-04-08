#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/BZideSaturation.h"
#include "DSP/BZideCompressor.h"
#include "DSP/BZideGate.h"
#include "DSP/BZideLimiter.h"
#include "DSP/DeEsserProcessor.h"
#include "LicenseValidator.h"

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

    // License
    bool isLicensed() const { return licenseValidator_.isLicensed(); }
    bool isTrial() const { return licenseValidator_.isTrial(); }
    bool isTrialExpired() const { return licenseValidator_.getStatus() == LicenseValidator::Status::Expired; }
    LicenseValidator::Status getLicenseStatus() const { return licenseValidator_.getStatus(); }
    int getTrialDaysRemaining() const { return licenseValidator_.getTrialDaysRemaining(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Section names for parameters
    static juce::String getPreParamId(const juce::String& param) { return "pre_" + param; }
    static juce::String getEqParamId(const juce::String& param) { return "eq_" + param; }
    static juce::String getDsParamId(const juce::String& param) { return "ds_" + param; }
    static juce::String getCompParamId(const juce::String& param) { return "comp_" + param; }
    static juce::String getGateParamId(const juce::String& param) { return "gate_" + param; }
    static juce::String getOutParamId(const juce::String& param) { return "out_" + param; }

    // Section processing order (for drag-to-reorder)
    // Values: 0=PRE, 1=EQ, 2=DS2, 3=COMP, 4=GATE
    std::atomic<int> sectionOrder_[5] = { 0, 1, 2, 3, 4 };
    void setSectionOrder(const std::array<int, 5>& order);

    // Individual DSP processing methods
    void processPre(juce::AudioBuffer<float>& buffer);
    void processEQ(juce::AudioBuffer<float>& buffer);
    void processDS2(juce::AudioBuffer<float>& buffer);
    void processComp(juce::AudioBuffer<float>& buffer);
    void processGate(juce::AudioBuffer<float>& buffer);

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

    void updateEQ();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BZideProcessor)
};

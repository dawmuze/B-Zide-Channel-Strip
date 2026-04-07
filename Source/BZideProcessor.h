#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/PreampProcessor.h"
#include "DSP/CompressorProcessor.h"
#include "DSP/GateProcessor.h"
#include "DSP/SaturationProcessor.h"
#include "DSP/ClipperProcessor.h"
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

private:
    juce::AudioProcessorValueTreeState apvts;
    LicenseValidator licenseValidator_;

    // DSP processors
    SaturationProcessor saturation_;
    DeEsserProcessor deEsser_;
    CompressorProcessor compressor_;
    GateProcessor gate_;
    ClipperProcessor limiter_;

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

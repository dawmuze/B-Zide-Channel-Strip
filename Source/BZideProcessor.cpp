#include "BZideProcessor.h"
#include "BZideEditor.h"

BZideProcessor::BZideProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "BZideParams", createParameterLayout())
{
    // License: auto-start trial
    if (licenseValidator_.getStatus() == LicenseValidator::Status::NotActivated)
        licenseValidator_.startTrial();
    else if (licenseValidator_.isTrial())
        licenseValidator_.checkTrial();
}

BZideProcessor::~BZideProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout BZideProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── PRE Section ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("pre_bypass", 1), "PRE Bypass", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("pre_type", 1), "PRE Type",
        juce::StringArray{ "ODD", "EVEN", "HEAVY" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pre_drive", 1), "PRE Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pre_tone", 1), "PRE Tone",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pre_input", 1), "PRE Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pre_mix", 1), "PRE Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pre_output", 1), "PRE Output",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    // ── EQ Section ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("eq_bypass", 1), "EQ Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_hpf", 1), "EQ HPF",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.3f), 20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_lpf", 1), "EQ LPF",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_low_freq", 1), "EQ Low Freq",
        juce::NormalisableRange<float>(30.0f, 500.0f, 1.0f, 0.3f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_low_gain", 1), "EQ Low Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_mid_freq", 1), "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.3f), 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_mid_gain", 1), "EQ Mid Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_mid_q", 1), "EQ Mid Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_high_freq", 1), "EQ High Freq",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.3f), 8000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_high_gain", 1), "EQ High Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));

    // ── DS² (De-Esser) Section ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("ds_bypass", 1), "DS Bypass", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ds_freq1", 1), "DS Band1 Freq",
        juce::NormalisableRange<float>(2000.0f, 12000.0f, 1.0f, 0.3f), 6000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ds_thresh1", 1), "DS Band1 Thresh",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ds_freq2", 1), "DS Band2 Freq",
        juce::NormalisableRange<float>(4000.0f, 20000.0f, 1.0f, 0.3f), 10000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ds_thresh2", 1), "DS Band2 Thresh",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f));

    // ── COMP Section ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("comp_bypass", 1), "COMP Bypass", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("comp_type", 1), "COMP Type",
        juce::StringArray{ "VCA", "FET", "OPT" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_threshold", 1), "COMP Threshold",
        juce::NormalisableRange<float>(-50.0f, 0.0f, 0.1f), -20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_ratio", 1), "COMP Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_attack", 1), "COMP Attack",
        juce::NormalisableRange<float>(0.1f, 150.0f, 0.1f, 0.3f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_release", 1), "COMP Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.3f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_makeup", 1), "COMP Makeup",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_mix", 1), "COMP Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("comp_output", 1), "COMP Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // ── GATE Section ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("gate_bypass", 1), "GATE Bypass", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("gate_type", 1), "GATE Type",
        juce::StringArray{ "GATE", "EXP" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gate_threshold", 1), "GATE Threshold",
        juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f), -40.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gate_atten", 1), "GATE Atten",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -30.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gate_floor", 1), "GATE Floor",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -60.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gate_attack", 1), "GATE Attack",
        juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.3f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gate_release", 1), "GATE Release",
        juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.3f), 200.0f));

    // ── OUTPUT Section ──
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("out_fader", 1), "Output Fader",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("out_limiter", 1), "Limiter On", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("out_limiter_thresh", 1), "Limiter Thresh",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.3f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("out_mode", 1), "Output Mode",
        juce::StringArray{ "Stereo", "Mono", "M/S" }, 0));

    return layout;
}

void BZideProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate_ = sampleRate;

    if (licenseValidator_.isTrial())
        licenseValidator_.checkTrial();

    saturation_.prepare(sampleRate, samplesPerBlock);
    deEsser_.prepare(sampleRate, samplesPerBlock);
    compressor_.prepare(sampleRate, samplesPerBlock);
    gate_.prepare(sampleRate, samplesPerBlock);
    limiter_.prepare(sampleRate, samplesPerBlock);

    // EQ
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;
    leftEQ_.prepare(spec);
    rightEQ_.prepare(spec);

    updateEQ();
}

void BZideProcessor::releaseResources() {}

void BZideProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // License check
    auto status = getLicenseStatus();
    if (!isLicensed() && !isTrial() &&
        (status == LicenseValidator::Status::Expired ||
         status == LicenseValidator::Status::Revoked ||
         status == LicenseValidator::Status::Locked))
    {
        buffer.clear();
        return;
    }

    // Input metering
    {
        float rmsL = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        float rmsR = buffer.getNumChannels() > 1 ? buffer.getRMSLevel(1, 0, buffer.getNumSamples()) : rmsL;
        inputLevelL.store(juce::Decibels::gainToDecibels(rmsL, -100.0f));
        inputLevelR.store(juce::Decibels::gainToDecibels(rmsR, -100.0f));
    }

    // Process DSP sections in user-defined order
    for (int i = 0; i < 5; ++i)
    {
        int sectionIdx = sectionOrder_[i].load();
        switch (sectionIdx)
        {
            case 0: processPre(buffer); break;
            case 1: processEQ(buffer); break;
            case 2: processDS2(buffer); break;
            case 3: processComp(buffer); break;
            case 4: processGate(buffer); break;
            default: break;
        }
    }

    // ── LIMITER ──
    {
        bool on = *apvts.getRawParameterValue("out_limiter") > 0.5f;
        limiter_.setBypass(!on);
        limiter_.setThreshold(*apvts.getRawParameterValue("out_limiter_thresh"));
        limiter_.process(buffer);
    }

    // ── OUTPUT FADER ──
    float faderDb = *apvts.getRawParameterValue("out_fader");
    float faderGain = juce::Decibels::decibelsToGain(faderDb);
    buffer.applyGain(faderGain);

    // ── OUTPUT MODE ──
    int outMode = static_cast<int>(*apvts.getRawParameterValue("out_mode"));
    if (outMode == 1 && buffer.getNumChannels() >= 2) // Mono
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float mono = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;
            buffer.setSample(0, i, mono);
            buffer.setSample(1, i, mono);
        }
    }
    else if (outMode == 2 && buffer.getNumChannels() >= 2) // M/S encode
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float l = buffer.getSample(0, i);
            float r = buffer.getSample(1, i);
            buffer.setSample(0, i, (l + r) * 0.5f); // Mid
            buffer.setSample(1, i, (l - r) * 0.5f); // Side
        }
    }

    // Output metering
    {
        float rmsL = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        float rmsR = buffer.getNumChannels() > 1 ? buffer.getRMSLevel(1, 0, buffer.getNumSamples()) : rmsL;
        outputLevelL.store(juce::Decibels::gainToDecibels(rmsL, -100.0f));
        outputLevelR.store(juce::Decibels::gainToDecibels(rmsR, -100.0f));
    }

    // Sanitize output
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float s = buffer.getSample(ch, i);
            buffer.setSample(ch, i, std::isfinite(s) ? s : 0.0f);
        }
}

void BZideProcessor::updateEQ()
{
    float hpf = *apvts.getRawParameterValue("eq_hpf");
    float lpf = *apvts.getRawParameterValue("eq_lpf");
    float lowF = *apvts.getRawParameterValue("eq_low_freq");
    float lowG = *apvts.getRawParameterValue("eq_low_gain");
    float midF = *apvts.getRawParameterValue("eq_mid_freq");
    float midG = *apvts.getRawParameterValue("eq_mid_gain");
    float midQ = *apvts.getRawParameterValue("eq_mid_q");
    float hiF = *apvts.getRawParameterValue("eq_high_freq");
    float hiG = *apvts.getRawParameterValue("eq_high_gain");

    auto sr = currentSampleRate_;
    if (sr <= 0) return;

    // HPF
    *leftEQ_.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hpf);
    *rightEQ_.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hpf);

    // Low shelf
    *leftEQ_.get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, lowF, 0.707f, juce::Decibels::decibelsToGain(lowG));
    *rightEQ_.get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, lowF, 0.707f, juce::Decibels::decibelsToGain(lowG));

    // Mid peak
    *leftEQ_.get<2>().coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, midF, midQ, juce::Decibels::decibelsToGain(midG));
    *rightEQ_.get<2>().coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, midF, midQ, juce::Decibels::decibelsToGain(midG));

    // High shelf
    *leftEQ_.get<3>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, hiF, 0.707f, juce::Decibels::decibelsToGain(hiG));
    *rightEQ_.get<3>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, hiF, 0.707f, juce::Decibels::decibelsToGain(hiG));

    // LPF
    *leftEQ_.get<4>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, lpf);
    *rightEQ_.get<4>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, lpf);
}

void BZideProcessor::setSectionOrder(const std::array<int, 5>& order)
{
    for (int i = 0; i < 5; ++i)
        sectionOrder_[i].store(order[(size_t)i]);
}

void BZideProcessor::processPre(juce::AudioBuffer<float>& buffer)
{
    bool bypassed = *apvts.getRawParameterValue("pre_bypass") > 0.5f;
    if (bypassed) return;

    // Input gain — drives the saturation harder or softer
    float inputDb = *apvts.getRawParameterValue("pre_input");
    float inputGain = juce::Decibels::decibelsToGain(inputDb);

    // Mix (dry/wet)
    float mixPct = *apvts.getRawParameterValue("pre_mix") / 100.0f;

    // Output compensation
    float outputDb = *apvts.getRawParameterValue("pre_output");
    float outputGain = juce::Decibels::decibelsToGain(outputDb);

    // Save dry signal for mix
    juce::AudioBuffer<float> dryBuffer;
    if (mixPct < 0.99f)
    {
        dryBuffer.makeCopyOf(buffer);
    }

    // Apply input gain
    buffer.applyGain(inputGain);

    // Process saturation
    saturation_.setBypass(false);
    saturation_.setDrive(*apvts.getRawParameterValue("pre_drive"));
    saturation_.setTone(*apvts.getRawParameterValue("pre_tone"));
    int preType = static_cast<int>(*apvts.getRawParameterValue("pre_type"));
    saturation_.setMode(static_cast<BZideSaturation::Mode>(preType));
    saturation_.process(buffer);

    // Mix dry/wet
    if (mixPct < 0.99f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                wet[i] = dry[i] * (1.0f - mixPct) + wet[i] * mixPct;
        }
    }

    // Apply output gain
    buffer.applyGain(outputGain);
}

void BZideProcessor::processEQ(juce::AudioBuffer<float>& buffer)
{
    bool eqBypassed = *apvts.getRawParameterValue("eq_bypass") > 0.5f;
    if (!eqBypassed)
    {
        updateEQ();
        auto blockL = juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock(0);
        auto blockR = buffer.getNumChannels() > 1
            ? juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock(1)
            : blockL;
        auto ctxL = juce::dsp::ProcessContextReplacing<float>(blockL);
        auto ctxR = juce::dsp::ProcessContextReplacing<float>(blockR);
        leftEQ_.process(ctxL);
        if (buffer.getNumChannels() > 1)
            rightEQ_.process(ctxR);
    }
}

void BZideProcessor::processDS2(juce::AudioBuffer<float>& buffer)
{
    bool dsBypassed = *apvts.getRawParameterValue("ds_bypass") > 0.5f;
    if (!dsBypassed)
    {
        deEsser_.setBand(0,
            *apvts.getRawParameterValue("ds_freq1"),
            *apvts.getRawParameterValue("ds_thresh1"), -12.0f);
        deEsser_.setBand(1,
            *apvts.getRawParameterValue("ds_freq2"),
            *apvts.getRawParameterValue("ds_thresh2"), -12.0f);
        deEsser_.process(buffer);
    }
}

void BZideProcessor::processComp(juce::AudioBuffer<float>& buffer)
{
    bool bypassed = *apvts.getRawParameterValue("comp_bypass") > 0.5f;
    compressor_.setBypass(bypassed);
    compressor_.setThreshold(*apvts.getRawParameterValue("comp_threshold"));
    compressor_.setRatio(*apvts.getRawParameterValue("comp_ratio"));
    compressor_.setAttack(*apvts.getRawParameterValue("comp_attack"));
    compressor_.setRelease(*apvts.getRawParameterValue("comp_release"));
    compressor_.setMakeupGain(*apvts.getRawParameterValue("comp_makeup"));
    compressor_.setMix(*apvts.getRawParameterValue("comp_mix"));
    int compType = static_cast<int>(*apvts.getRawParameterValue("comp_type"));
    compressor_.setModel(static_cast<BZideCompressor::Model>(compType));
    compressor_.process(buffer);
    gainReduction.store(compressor_.getGainReduction());
}

void BZideProcessor::processGate(juce::AudioBuffer<float>& buffer)
{
    bool bypassed = *apvts.getRawParameterValue("gate_bypass") > 0.5f;
    gate_.setBypass(bypassed);
    gate_.setThreshold(*apvts.getRawParameterValue("gate_threshold"));
    gate_.setAttenuation(*apvts.getRawParameterValue("gate_atten"));
    gate_.setFloor(*apvts.getRawParameterValue("gate_floor"));
    gate_.setAttack(*apvts.getRawParameterValue("gate_attack"));
    gate_.setRelease(*apvts.getRawParameterValue("gate_release"));
    int gateType = static_cast<int>(*apvts.getRawParameterValue("gate_type"));
    gate_.setMode(static_cast<BZideGate::Mode>(gateType));
    gate_.process(buffer);
}

void BZideProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Save section order
    auto* orderXml = xml->createNewChildElement("SectionOrder");
    for (int i = 0; i < 5; ++i)
        orderXml->setAttribute("s" + juce::String(i), sectionOrder_[i].load());

    copyXmlToBinary(*xml, destData);
}

void BZideProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        // Restore section order
        if (auto* orderXml = xml->getChildByName("SectionOrder"))
        {
            for (int i = 0; i < 5; ++i)
                sectionOrder_[i].store(orderXml->getIntAttribute("s" + juce::String(i), i));
        }

        // Remove the SectionOrder child before restoring APVTS state
        xml->deleteAllChildElementsWithTagName("SectionOrder");
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorEditor* BZideProcessor::createEditor()
{
    return new BZideEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BZideProcessor();
}

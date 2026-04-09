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
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_low_q", 1), "EQ Low Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 0.707f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("eq_high_q", 1), "EQ High Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 0.707f));

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
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ds_output", 1), "DS Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

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
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f), -10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("out_limiter_ceiling", 1), "Limiter Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("out_limiter_release", 1), "Limiter Release",
        juce::NormalisableRange<float>(0.01f, 100.0f, 0.01f, 0.3f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("out_phase_l", 1), "Phase Invert L", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("out_phase_r", 1), "Phase Invert R", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("out_mode", 1), "Output Mode",
        juce::StringArray{ "Stereo", "Mono", "M/S" }, 0));

    // ── Phase 1: EQ Curve Types ──
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("eq_high_type", 1), "EQ High Type", juce::StringArray{"Low Shelf","Bell","High Shelf"}, 2));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("eq_mid_type", 1), "EQ Mid Type", juce::StringArray{"Low Shelf","Bell","High Shelf"}, 1));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("eq_low_type", 1), "EQ Low Type", juce::StringArray{"Low Shelf","Bell","High Shelf"}, 0));

    // ── Phase 1: Pre-Section Filters ──
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("pre_hpf_slope", 1), "HPF Slope", juce::StringArray{"6","12","18"}, 1));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("pre_lpf_slope", 1), "LPF Slope", juce::StringArray{"6","12","18"}, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pre_lpf", 1), "Pre LPF", juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

    // ── Phase 2: Compressor Detection / Topology ──
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("comp_detect", 1), "Comp Detect", juce::StringArray{"RMS","Peak"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("comp_sc_hpf", 1), "Comp SC HPF", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("comp_topology", 1), "Comp Topology", juce::StringArray{"FF","FB"}, 0));

    // ── Phase 3: Gate Controls ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("gate_fast", 1), "Gate Fast", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("gate_peak", 1), "Gate Peak", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("gate_sc", 1), "Gate SC", false));

    // ── Phase 4: LOWRIDE Sub Bass ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("pre_lowride", 1), "Pre Lowride", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("pre_lowride_db", 1), "Lowride Amount", juce::StringArray{"2","4"}, 0));

    // ── Master Bypass (IN button) ──
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("master_bypass", 1), "Master Bypass", false));

    return layout;
}

void BZideProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Validate inputs (some hosts pass 0 or invalid values)
    if (sampleRate <= 0) sampleRate = 44100.0;
    if (samplesPerBlock <= 0) samplesPerBlock = 512;
    currentSampleRate_ = sampleRate;

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

    // LOWRIDE filters
    lowrideL_.reset();
    lowrideR_.reset();
    auto lrCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 40.0, 0.707, 1.0f);
    lowrideL_.coefficients = lrCoeffs;
    lowrideR_.coefficients = lrCoeffs;

    // Pre LPF filters
    preLpfL_.reset();
    preLpfR_.reset();
    auto preLpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);
    preLpfL_.coefficients = preLpfCoeffs;
    preLpfR_.coefficients = preLpfCoeffs;

    // Pre-allocate dry buffer (FIX 2)
    preDryBuffer_.setSize(2, samplesPerBlock);

    // Report total latency to host for PDC (plugin delay compensation)
    // Oversampling (anti-aliasing filters) + Limiter look-ahead
    int totalLatency = limiter_.getLatencySamples() + saturation_.getLatencySamples();
    setLatencySamples(totalLatency);

    // Re-prepare loaded insert slots to reset internal state between renders
    for (int i = 0; i < numInsertSlots; ++i)
    {
        if (insertSlots_[i].isLoaded())
        {
            auto type = insertSlots_[i].getModuleType();
            insertSlots_[i].unload();
            insertSlots_[i].loadModule(type, sampleRate, samplesPerBlock);
        }
    }

    // Reset cached params so coefficients are recalculated on first block
    cachedHpf_ = -1; cachedLpf_ = -1;
    cachedLowF_ = -1; cachedLowG_ = -1; cachedLowQ_ = -1; cachedLowType_ = -1;
    cachedMidF_ = -1; cachedMidG_ = -1; cachedMidQ_ = -1; cachedMidType_ = -1;
    cachedHiF_ = -1; cachedHiG_ = -1; cachedHiQ_ = -1; cachedHiType_ = -1;
    cachedHpfSlope_ = -1; cachedLpfSlope_ = -1;
    cachedLowrideBoost_ = -1;
    cachedPreLpf_ = -1;

    // Reset previous gain values for smooth ramping
    prevInputGain_ = 1.0f;
    prevOutputGain_ = 1.0f;
    prevDsOutputGain_ = 1.0f;
    prevCompOutputGain_ = 1.0f;
    prevFaderGain_ = 1.0f;

    updateEQ();
}

void BZideProcessor::releaseResources() {}

void BZideProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Guard: empty buffer (some hosts send 0-sample blocks)
    if (buffer.getNumSamples() == 0) return;

    // Guard: buffer larger than prepared — resize pre-allocated buffers
    if (buffer.getNumSamples() > preDryBuffer_.getNumSamples())
        preDryBuffer_.setSize(juce::jmax(2, buffer.getNumChannels()),
                              buffer.getNumSamples(), false, false, true);

    // Apply pending insert swap on audio thread (FIX 8)
    {
        int swapA = pendingSwapA_.exchange(-1);
        int swapB = pendingSwapB_.exchange(-1);
        if (swapA >= 0 && swapB >= 0 && swapA < numInsertSlots && swapB < numInsertSlots)
            std::swap(insertSlots_[swapA], insertSlots_[swapB]);
    }

    // License check — with 50ms crossfade to avoid pop on status transition
    auto licStatus = getLicenseStatus();
    bool licenseLocked = !isLicensed() && !isTrial() &&
        (licStatus == LicenseValidator::Status::Expired ||
         licStatus == LicenseValidator::Status::Revoked ||
         licStatus == LicenseValidator::Status::Locked);

    float licTarget = licenseLocked ? 0.0f : 1.0f;
    if (std::abs(licenseFadeGain_ - licTarget) > 0.001f || licenseLocked)
    {
        float fadeStep = 1.0f / (float)(currentSampleRate_ * 0.050); // 50ms fade
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (licenseFadeGain_ > licTarget)
                licenseFadeGain_ = juce::jmax(licenseFadeGain_ - fadeStep, 0.0f);
            else if (licenseFadeGain_ < licTarget)
                licenseFadeGain_ = juce::jmin(licenseFadeGain_ + fadeStep, 1.0f);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * licenseFadeGain_);
        }
        if (licenseFadeGain_ <= 0.0f)
            return; // fully faded out, skip remaining processing
    }

    // Input metering
    {
        float rmsL = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        float rmsR = buffer.getNumChannels() > 1 ? buffer.getRMSLevel(1, 0, buffer.getNumSamples()) : rmsL;
        inputLevelL.store(juce::Decibels::gainToDecibels(rmsL, -100.0f));
        inputLevelR.store(juce::Decibels::gainToDecibels(rmsR, -100.0f));
    }

    // Master bypass with crossfade (~10ms ramp, no pop)
    bool isBypassed = *apvts.getRawParameterValue("master_bypass") > 0.5f;
    float bypassTarget = isBypassed ? 0.0f : 1.0f;

    // Save dry input for bypass crossfade (reuse pre-allocated buffer)
    bool needsCrossfade = (std::abs(bypassGain_ - bypassTarget) > 0.001f) || (isBypassed && bypassGain_ > 0.001f);
    if (needsCrossfade)
    {
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), preDryBuffer_.getNumChannels()); ++ch)
            preDryBuffer_.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

    // Process DSP sections in user-defined order (6 draggable sections)
    for (int i = 0; i < 6; ++i)
    {
        int sectionIdx = sectionOrder_[i].load();
        switch (sectionIdx)
        {
            case 0: processPre(buffer); break;
            case 1: processEQ(buffer); break;
            case 2: processDS2(buffer); break;
            case 3: processComp(buffer); break;
            case 4: processGate(buffer); break;
            case 5: processInserts(buffer); break;
            default: break;
        }
    }

    // Phase 3A: Soft-clip protection between sections and output
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float s = buffer.getSample(ch, i);
                if (std::abs(s) > 4.0f)
                    buffer.setSample(ch, i, std::tanh(s * 0.25f) * 4.0f);
            }
    }

    // ── PHASE INVERT ──
    {
        bool phaseL = *apvts.getRawParameterValue("out_phase_l") > 0.5f;
        bool phaseR = *apvts.getRawParameterValue("out_phase_r") > 0.5f;
        if (phaseL && buffer.getNumChannels() > 0)
            juce::FloatVectorOperations::negate(buffer.getWritePointer(0), buffer.getWritePointer(0), buffer.getNumSamples());
        if (phaseR && buffer.getNumChannels() > 1)
            juce::FloatVectorOperations::negate(buffer.getWritePointer(1), buffer.getWritePointer(1), buffer.getNumSamples());
    }

    // Phase 3C: Output fader BEFORE limiter (ramped to avoid clicks on automation)
    float faderDb = *apvts.getRawParameterValue("out_fader");
    float faderGain = juce::Decibels::decibelsToGain(faderDb);
    buffer.applyGainRamp(0, buffer.getNumSamples(), prevFaderGain_, faderGain);
    prevFaderGain_ = faderGain;

    // ── LIMITER ──
    {
        bool on = *apvts.getRawParameterValue("out_limiter") > 0.5f;
        limiter_.setBypass(!on);
        limiter_.setThreshold(*apvts.getRawParameterValue("out_limiter_thresh"));
        limiter_.setRelease(*apvts.getRawParameterValue("out_limiter_release"));
        limiter_.process(buffer);

        // Apply ceiling (output ceiling caps the final level)
        float ceiling = juce::Decibels::decibelsToGain((float)*apvts.getRawParameterValue("out_limiter_ceiling"));
        if (on)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    float s = buffer.getSample(ch, i);
                    if (std::abs(s) > ceiling)
                        buffer.setSample(ch, i, s > 0 ? ceiling : -ceiling);
                }
        }
    }

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

    // ── BYPASS CROSSFADE (~10ms ramp) ──
    if (needsCrossfade)
    {
        float rampStep = 1.0f / (float)(currentSampleRate_ * 0.010); // 10ms
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Ramp bypassGain_ toward target
            if (bypassGain_ < bypassTarget)
                bypassGain_ = juce::jmin(bypassGain_ + rampStep, bypassTarget);
            else if (bypassGain_ > bypassTarget)
                bypassGain_ = juce::jmax(bypassGain_ - rampStep, bypassTarget);

            float wet = bypassGain_;
            float dry = 1.0f - wet;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float dryS = preDryBuffer_.getSample(ch, i);
                float wetS = buffer.getSample(ch, i);
                buffer.setSample(ch, i, dryS * dry + wetS * wet);
            }
        }
    }

    // Push samples into FFT FIFO for spectrum analyzer
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        pushNextSampleIntoFifo(buffer.getSample(0, i));

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
    float lowQ = *apvts.getRawParameterValue("eq_low_q");
    float hiQ = *apvts.getRawParameterValue("eq_high_q");

    int lowType = static_cast<int>(*apvts.getRawParameterValue("eq_low_type"));
    int midType = static_cast<int>(*apvts.getRawParameterValue("eq_mid_type"));
    int highType = static_cast<int>(*apvts.getRawParameterValue("eq_high_type"));

    int hpfSlope = static_cast<int>(*apvts.getRawParameterValue("pre_hpf_slope"));
    int lpfSlope = static_cast<int>(*apvts.getRawParameterValue("pre_lpf_slope"));

    auto sr = currentSampleRate_;
    if (sr <= 0) return;

    // FIX 1: Only recalculate coefficients for bands whose params actually changed

    // HPF — slope controls Q: 0=6dB(gentle), 1=12dB(Butterworth), 2=18dB(steep)
    if (cachedHpf_ < 0.0f || std::abs(hpf - cachedHpf_) > 0.5f || cachedHpfSlope_ != hpfSlope)
    {
        cachedHpf_ = hpf;
        cachedHpfSlope_ = hpfSlope;
        float hpfQ = (hpfSlope == 0) ? 0.5f : (hpfSlope == 2) ? 1.3f : 0.707f;
        auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hpf, hpfQ);
        *leftEQ_.get<0>().coefficients = *hpfCoeffs;
        *rightEQ_.get<0>().coefficients = *hpfCoeffs;
    }

    // Low band — dynamic curve type
    if (cachedLowF_ < 0.0f || std::abs(lowF - cachedLowF_) > 0.5f || std::abs(lowG - cachedLowG_) > 0.05f ||
        std::abs(lowQ - cachedLowQ_) > 0.005f || lowType != cachedLowType_)
    {
        cachedLowF_ = lowF; cachedLowG_ = lowG; cachedLowQ_ = lowQ; cachedLowType_ = lowType;
        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;
        if (lowType == 0)
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, lowF, lowQ, juce::Decibels::decibelsToGain(lowG));
        else if (lowType == 1)
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, lowF, lowQ, juce::Decibels::decibelsToGain(lowG));
        else
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, lowF, lowQ, juce::Decibels::decibelsToGain(lowG));
        *leftEQ_.get<1>().coefficients = *coeffs;
        *rightEQ_.get<1>().coefficients = *coeffs;
    }

    // Mid band — dynamic curve type
    if (cachedMidF_ < 0.0f || std::abs(midF - cachedMidF_) > 0.5f || std::abs(midG - cachedMidG_) > 0.05f ||
        std::abs(midQ - cachedMidQ_) > 0.005f || midType != cachedMidType_)
    {
        cachedMidF_ = midF; cachedMidG_ = midG; cachedMidQ_ = midQ; cachedMidType_ = midType;
        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;
        if (midType == 0)
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, midF, midQ, juce::Decibels::decibelsToGain(midG));
        else if (midType == 1)
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, midF, midQ, juce::Decibels::decibelsToGain(midG));
        else
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, midF, midQ, juce::Decibels::decibelsToGain(midG));
        *leftEQ_.get<2>().coefficients = *coeffs;
        *rightEQ_.get<2>().coefficients = *coeffs;
    }

    // High band — dynamic curve type
    if (cachedHiF_ < 0.0f || std::abs(hiF - cachedHiF_) > 0.5f || std::abs(hiG - cachedHiG_) > 0.05f ||
        std::abs(hiQ - cachedHiQ_) > 0.005f || highType != cachedHiType_)
    {
        cachedHiF_ = hiF; cachedHiG_ = hiG; cachedHiQ_ = hiQ; cachedHiType_ = highType;
        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;
        if (highType == 0)
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, hiF, hiQ, juce::Decibels::decibelsToGain(hiG));
        else if (highType == 1)
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, hiF, hiQ, juce::Decibels::decibelsToGain(hiG));
        else
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, hiF, hiQ, juce::Decibels::decibelsToGain(hiG));
        *leftEQ_.get<3>().coefficients = *coeffs;
        *rightEQ_.get<3>().coefficients = *coeffs;
    }

    // LPF — slope controls Q: 0=6dB(gentle), 1=12dB(Butterworth), 2=18dB(steep)
    if (cachedLpf_ < 0.0f || std::abs(lpf - cachedLpf_) > 0.5f || cachedLpfSlope_ != lpfSlope)
    {
        cachedLpf_ = lpf;
        cachedLpfSlope_ = lpfSlope;
        float lpfQ = (lpfSlope == 0) ? 0.5f : (lpfSlope == 2) ? 1.3f : 0.707f;
        auto lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, lpf, lpfQ);
        *leftEQ_.get<4>().coefficients = *lpfCoeffs;
        *rightEQ_.get<4>().coefficients = *lpfCoeffs;
    }
}

void BZideProcessor::setSectionOrder(const std::array<int, 6>& order)
{
    for (int i = 0; i < 6; ++i)
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

    // Save dry signal for mix (FIX 2: use pre-allocated buffer, no heap alloc)
    if (mixPct < 0.99f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            preDryBuffer_.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

    // Apply input gain (ramped to avoid clicks on automation)
    buffer.applyGainRamp(0, buffer.getNumSamples(), prevInputGain_, inputGain);
    prevInputGain_ = inputGain;

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
            auto* dry = preDryBuffer_.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                wet[i] = dry[i] * (1.0f - mixPct) + wet[i] * mixPct;
        }
    }

    // LOWRIDE sub bass boost (Phase 4)
    bool lowride = *apvts.getRawParameterValue("pre_lowride") > 0.5f;
    if (lowride)
    {
        int lrDb = static_cast<int>(*apvts.getRawParameterValue("pre_lowride_db"));
        float boost = (lrDb == 0) ? 2.0f : 4.0f;
        // FIX 4: only recalculate coefficients when boost changes
        if (std::abs(boost - cachedLowrideBoost_) > 0.01f)
        {
            cachedLowrideBoost_ = boost;
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate_, 40.0, 0.707, juce::Decibels::decibelsToGain(boost));
            *lowrideL_.coefficients = *coeffs;
            *lowrideR_.coefficients = *coeffs;
        }

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            buffer.setSample(0, i, lowrideL_.processSample(buffer.getSample(0, i)));
            if (buffer.getNumChannels() > 1)
                buffer.setSample(1, i, lowrideR_.processSample(buffer.getSample(1, i)));
        }
    }

    // Pre LPF — only apply if not fully open
    float lpfFreq = *apvts.getRawParameterValue("pre_lpf");
    if (lpfFreq < 19999.0f)
    {
        // FIX 5: only recalculate coefficients when freq changes
        if (std::abs(lpfFreq - cachedPreLpf_) > 1.0f)
        {
            cachedPreLpf_ = lpfFreq;
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate_, lpfFreq);
            *preLpfL_.coefficients = *coeffs;
            *preLpfR_.coefficients = *coeffs;
        }

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            buffer.setSample(0, i, preLpfL_.processSample(buffer.getSample(0, i)));
            if (buffer.getNumChannels() > 1)
                buffer.setSample(1, i, preLpfR_.processSample(buffer.getSample(1, i)));
        }
    }

    // Apply output gain (ramped to avoid clicks on automation)
    buffer.applyGainRamp(0, buffer.getNumSamples(), prevOutputGain_, outputGain);
    prevOutputGain_ = outputGain;
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

        // Apply output gain (ramped to avoid clicks on automation)
        float dsOutGain = juce::Decibels::decibelsToGain((float)*apvts.getRawParameterValue("ds_output"));
        buffer.applyGainRamp(0, buffer.getNumSamples(), prevDsOutputGain_, dsOutGain);
        prevDsOutputGain_ = dsOutGain;
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

    // Phase 2: detection mode, SC HPF, topology
    int detectMode = static_cast<int>(*apvts.getRawParameterValue("comp_detect"));
    compressor_.setDetectMode(static_cast<BZideCompressor::DetectMode>(detectMode));
    bool scHpf = *apvts.getRawParameterValue("comp_sc_hpf") > 0.5f;
    compressor_.setScHpfEnabled(scHpf);
    int topology = static_cast<int>(*apvts.getRawParameterValue("comp_topology"));
    compressor_.setTopology(static_cast<BZideCompressor::Topology>(topology));

    compressor_.process(buffer);
    gainReduction.store(compressor_.getGainReduction());

    // Apply comp output gain (ramped to avoid clicks on automation)
    float compOutGain = juce::Decibels::decibelsToGain((float)*apvts.getRawParameterValue("comp_output"));
    buffer.applyGainRamp(0, buffer.getNumSamples(), prevCompOutputGain_, compOutGain);
    prevCompOutputGain_ = compOutGain;
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

    // Phase 3: fast, peak/RMS, sidechain
    gate_.setFast(*apvts.getRawParameterValue("gate_fast") > 0.5f);
    gate_.setPeakDetect(*apvts.getRawParameterValue("gate_peak") > 0.5f);
    gate_.setScEnabled(*apvts.getRawParameterValue("gate_sc") > 0.5f);

    gate_.process(buffer);
}

void BZideProcessor::pushNextSampleIntoFifo(float sample)
{
    int idx = fifoIndex.load();
    if (idx == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::copy(fifo, fifo + fftSize, fftData);
            nextFFTBlockReady = true;
        }
        idx = 0;
    }
    fifo[idx] = sample;
    fifoIndex.store(idx + 1);
}

// ── Insert slot management ──────────────────────────────────────────
void BZideProcessor::loadInsert(int slotIndex, InsertProcessor::ModuleType type)
{
    if (slotIndex < 0 || slotIndex >= numInsertSlots) return;
    insertSlots_[slotIndex].loadModule(type, currentSampleRate_, getBlockSize());
}

void BZideProcessor::removeInsert(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= numInsertSlots) return;
    insertSlots_[slotIndex].unload();
}

void BZideProcessor::setInsertBypass(int slotIndex, bool bypassed)
{
    if (slotIndex < 0 || slotIndex >= numInsertSlots) return;
    insertSlots_[slotIndex].setBypass(bypassed);
}

void BZideProcessor::swapInserts(int indexA, int indexB)
{
    if (indexA < 0 || indexA >= numInsertSlots) return;
    if (indexB < 0 || indexB >= numInsertSlots) return;
    if (indexA == indexB) return;
    // FIX 8: defer swap to audio thread to avoid race condition
    pendingSwapA_.store(indexA);
    pendingSwapB_.store(indexB);
}

void BZideProcessor::processInserts(juce::AudioBuffer<float>& buffer)
{
    for (int i = 0; i < numInsertSlots; ++i)
        insertSlots_[i].process(buffer);
}

// ── State persistence ───────────────────────────────────────────────
void BZideProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // State version for backward compatibility
    auto* versionXml = xml->createNewChildElement("PluginVersion");
    versionXml->setAttribute("major", 1);
    versionXml->setAttribute("minor", 0);
    versionXml->setAttribute("patch", 0);
    versionXml->setAttribute("paramCount", 64);

    // Save section order
    auto* orderXml = xml->createNewChildElement("SectionOrder");
    for (int i = 0; i < 6; ++i)
        orderXml->setAttribute("s" + juce::String(i), sectionOrder_[i].load());

    // Save insert slots
    auto* insertsXml = xml->createNewChildElement("Inserts");
    for (int i = 0; i < numInsertSlots; ++i)
    {
        auto* slotXml = insertsXml->createNewChildElement("Slot");
        slotXml->setAttribute("index", i);
        slotXml->setAttribute("module", static_cast<int>(insertSlots_[i].getModuleType()));
        slotXml->setAttribute("bypassed", insertSlots_[i].isBypassed());
    }

    copyXmlToBinary(*xml, destData);
}

void BZideProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        // Restore section order (with validation)
        if (auto* orderXml = xml->getChildByName("SectionOrder"))
        {
            for (int i = 0; i < 6; ++i)
            {
                int val = orderXml->getIntAttribute("s" + juce::String(i), i);
                if (val < 0 || val > 5) val = i; // clamp invalid indices to default
                sectionOrder_[i].store(val);
            }

            // Validate permutation: check for duplicates
            bool seen[6] = {};
            bool valid = true;
            for (int i = 0; i < 6; ++i)
            {
                int v = sectionOrder_[i].load();
                if (seen[v]) { valid = false; break; }
                seen[v] = true;
            }
            if (!valid) // reset to default order if duplicates found
                for (int i = 0; i < 6; ++i)
                    sectionOrder_[i].store(i);
        }

        // Restore insert slots (with enum validation)
        if (auto* insertsXml = xml->getChildByName("Inserts"))
        {
            for (auto* slotXml : insertsXml->getChildIterator())
            {
                int idx = slotXml->getIntAttribute("index", -1);
                int mod = slotXml->getIntAttribute("module", 0);
                bool byp = slotXml->getBoolAttribute("bypassed", false);

                // Validate: index in range, module type in valid enum range [1..6]
                if (idx >= 0 && idx < numInsertSlots && mod >= 1 && mod <= 6)
                {
                    double sr = currentSampleRate_ > 0 ? currentSampleRate_ : 44100.0;
                    int bs = getBlockSize() > 0 ? getBlockSize() : 512;
                    insertSlots_[idx].loadModule(
                        static_cast<InsertProcessor::ModuleType>(mod), sr, bs);
                    insertSlots_[idx].setBypass(byp);
                }
            }
        }

        // Remove custom children before restoring APVTS state
        xml->deleteAllChildElementsWithTagName("PluginVersion");
        xml->deleteAllChildElementsWithTagName("SectionOrder");
        xml->deleteAllChildElementsWithTagName("Inserts");
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

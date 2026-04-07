#include "LA2ACompressor.h"

LA2ACompressor::LA2ACompressor() {}

void LA2ACompressor::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 1;

    hpfL.prepare (spec); hpfR.prepare (spec);
    midL.prepare (spec); midR.prepare (spec);
    hiL.prepare  (spec); hiR.prepare  (spec);
    lpfL.prepare (spec); lpfR.prepare (spec);

    outSmoothCoeff = 1.f - std::exp (-2200.f / 10.f / static_cast<float> (sr));
    updateCoefficients();
    updateEQCoefficients();
    reset();
}

void LA2ACompressor::reset()
{
    opticalGain = 1.f;
    fastEnvelope = 1.f;
    slowEnvelope = 1.f;
    currentGR = 0.f;
    outputLevel = -60.f;

    smoothedDriveRange = 0.f;

    hpfL.reset(); hpfR.reset();
    midL.reset(); midR.reset();
    hiL.reset();  hiR.reset();
    lpfL.reset(); lpfR.reset();
}

void LA2ACompressor::updateCoefficients()
{
    float sr = static_cast<float> (sampleRate);
    float pr = peakReduction;

    currentThreshold = calOffset - std::pow (pr, calExp) * calMult;
    currentThresholdGain = juce::Decibels::decibelsToGain (currentThreshold);

    if (pr < 0.01f)
    {
        currentRatio = 1.f;
    }
    else if (compMode)
    {
        // ── COMPRESS ── soft knee, gentle like real LA-2A
        currentRatio = 1.5f + pr * 1.0f;           // 1.5:1 → 2.5:1
    }
    else
    {
        // ── LIMIT ── hard knee, aggressive, ratio 12:1–∞
        currentRatio = 12.f + pr * 88.f;            // 12:1 → 100:1 (≈∞)
    }

    float attackMs, releaseMs;

    if (compMode)
    {
        // COMPRESS: LA-2A style — slow optical attack lets transients through
        attackMs  = 30.f - pr * 15.f;               // 30–15 ms (optical lag)
        if (attackMs < 15.f) attackMs = 15.f;
        releaseMs = 600.f - pr * 200.f;             // 600–400 ms (musical tail)
    }
    else
    {
        // LIMIT: faster attack, still some transient pass-through
        attackMs  = 5.f - pr * 3.f;                 // 5–2 ms
        if (attackMs < 2.f) attackMs = 2.f;
        releaseMs = 400.f - pr * 200.f;             // 400–200 ms
    }

    // 700.f = gentle optical-style envelope (photocell inertia)
    opticalAttack        = 1.f - std::exp (-700.f / attackMs / sr);
    opticalReleaseFast   = 1.f - std::exp (-700.f / releaseMs / sr);
    opticalReleaseSlow   = 1.f - std::exp (-700.f / (releaseMs * 4.f) / sr);
}

void LA2ACompressor::updateEQCoefficients()
{
    // Safety: clamp all filter frequencies to well below Nyquist
    float nyquist = static_cast<float> (sampleRate) * 0.499f;

    // HPF – 2nd-order Butterworth high-pass
    float safeHpf = juce::jlimit (20.f, juce::jmin (nyquist * 0.9f, 500.f), eqHpfFreq);
    auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        sampleRate, safeHpf, 0.707f);
    *hpfL.coefficients = *hpfCoeffs;
    *hpfR.coefficients = *hpfCoeffs;

    // Peak / bell
    float safeMid = juce::jlimit (200.f, juce::jmin (nyquist * 0.9f, 8000.f), eqMidFreq);
    float midGainLin = juce::Decibels::decibelsToGain (eqMidGainDb);
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, safeMid, 1.5f, midGainLin);
    *midL.coefficients = *midCoeffs;
    *midR.coefficients = *midCoeffs;

    // High shelf
    float safeHi = juce::jlimit (2000.f, juce::jmin (nyquist * 0.9f, 20000.f), eqHiFreq);
    float hiGainLin = juce::Decibels::decibelsToGain (eqHiGainDb);
    auto hiCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, safeHi, 0.707f, hiGainLin);
    *hiL.coefficients = *hiCoeffs;
    *hiR.coefficients = *hiCoeffs;

    // Low-pass filter – 2nd-order Butterworth
    float safeLpf = juce::jlimit (2000.f, juce::jmin (nyquist * 0.9f, 20000.f), eqLpfFreq);
    auto lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (
        sampleRate, safeLpf, 0.707f);
    *lpfL.coefficients = *lpfCoeffs;
    *lpfR.coefficients = *lpfCoeffs;
}

float LA2ACompressor::applySoftSaturation (float sample)
{
    if (!tubeEnabled) return sample;

    // LA-2A style tube stage: 12AX7 triode adds warmth + body
    // 1. Soft saturation (odd harmonics — 3rd, 5th)
    float drive = 1.6f;
    float sat = std::tanh (sample * drive) / std::tanh (drive);

    // 2. Even harmonic generation (2nd harmonic — warmth/body)
    //    Asymmetric transfer: positive half clips slightly different than negative
    float even = 0.05f * sample * std::abs (sample);

    // 3. Subtle bias shift — adds fullness to low end (transformer coupling)
    float bias = 0.01f * sample;

    return sat + even + bias;
}

float LA2ACompressor::applyOutputStage (float sample)
{
    // +4 (driveRange≈0): clean passthrough, no saturation
    // +10 (driveRange≈1): drive scales with Gain knob → saturation
    if (smoothedDriveRange < 0.01f) return sample;

    // Drive depends on BOTH the output range AND the Gain knob position
    // At Gain=0: drive=1 (clean even in +10) → no level change on switch
    // At Gain=max: drive up to 2.5 → progressive saturation
    float drive = 1.f + smoothedDriveRange * gainNormalized * 1.5f;
    if (drive <= 1.01f) return sample;

    // Symmetric soft clip (odd harmonics: 3rd, 5th)
    float d = sample * drive;
    float sat = std::tanh (d) / std::tanh (drive);
    // Asymmetric warmth (even harmonics: subtle 2nd harmonic)
    sat += 0.04f * sample * sample * std::exp (-2.f * std::abs (sample));
    return sat;
}

void LA2ACompressor::setPeakReduction (float normalized)
{
    peakReduction = juce::jlimit (0.f, 1.f, normalized);
    updateCoefficients();
}

void LA2ACompressor::setGain (float normalized)
{
    gainNormalized = normalized;
    makeupGain = juce::Decibels::decibelsToGain (normalized * 20.f);
}

void LA2ACompressor::setCompressMode (bool compress) { compMode = compress; updateCoefficients(); }
void LA2ACompressor::setFastMode    (bool fast)      { fastRelease = fast;  updateCoefficients(); }

void LA2ACompressor::setHPF (float freqHz)
{
    if (std::abs (freqHz - eqHpfFreq) > 0.5f)
    {
        eqHpfFreq = freqHz;
        updateEQCoefficients();
    }
}

void LA2ACompressor::setEQMid (float freqHz, float gainDb)
{
    if (std::abs (freqHz - eqMidFreq) > 0.5f || std::abs (gainDb - eqMidGainDb) > 0.05f)
    {
        eqMidFreq   = freqHz;
        eqMidGainDb = gainDb;
        updateEQCoefficients();
    }
}

void LA2ACompressor::setEQHi (float freqHz, float gainDb)
{
    if (std::abs (freqHz - eqHiFreq) > 0.5f || std::abs (gainDb - eqHiGainDb) > 0.05f)
    {
        eqHiFreq   = freqHz;
        eqHiGainDb = gainDb;
        updateEQCoefficients();
    }
}

void LA2ACompressor::setLPF (float freqHz)
{
    if (std::abs (freqHz - eqLpfFreq) > 0.5f)
    {
        eqLpfFreq = freqHz;
        updateEQCoefficients();
    }
}

void LA2ACompressor::setEQBypass   (bool bypassed)    { eqBypassed  = bypassed; }
void LA2ACompressor::setMix        (float normalized) { mixAmount    = juce::jlimit (0.f, 1.f, normalized); }
void LA2ACompressor::setStereoLink (bool linked)      { stereoLinked = linked; }
void LA2ACompressor::setTubeMode   (bool tube)        { tubeEnabled  = tube;   }
void LA2ACompressor::setOutputRange (bool is10)       { outputRange10 = is10;  }

void LA2ACompressor::setCalibration (float offset, float mult, float exp)
{
    calOffset = offset;
    calMult   = mult;
    calExp    = exp;
}

void LA2ACompressor::process (juce::AudioBuffer<float>& buffer)
{
    int numSamples  = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    if (numChannels == 0 || numSamples == 0) return;

    // ── Apply EQ pre-compression (skip if bypassed) ──
    if (!eqBypassed && numChannels > 0)
    {
        auto* chL = buffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
            chL[i] = lpfL.processSample (hpfL.processSample (midL.processSample (hiL.processSample (chL[i]))));
    }
    if (!eqBypassed && numChannels > 1)
    {
        auto* chR = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
            chR[i] = lpfR.processSample (hpfR.processSample (midR.processSample (hiR.processSample (chR[i]))));
    }

    // ── Optical compressor ──
    float grMax   = 0.f;
    float outPeak = 0.f;
    float thGain  = currentThresholdGain;
    float compSlope = 1.f - 1.f / currentRatio;

    for (int i = 0; i < numSamples; i++)
    {
        float inL = (numChannels > 0) ? buffer.getSample (0, i) : 0.f;
        float inR = (numChannels > 1) ? buffer.getSample (1, i) : inL;

        float inLevel = stereoLinked
            ? juce::jmax (std::abs (inL), std::abs (inR))
            : std::abs (inL);

        float targetGain = 1.f;
        if (peakReduction > 0.001f && thGain > 0.0001f && inLevel > 0.0001f)
            targetGain = std::fmin (1.f, std::pow (inLevel / thGain, -compSlope));

        // ── Dual-stage optical release (LA-2A style) ──
        // Fast envelope: quick attack, fast initial release
        float fastCoeff = (targetGain < fastEnvelope) ? opticalAttack : opticalReleaseFast;
        fastEnvelope = targetGain * fastCoeff + fastEnvelope * (1.f - fastCoeff);

        // Slow envelope: follows fast but releases much slower (4x)
        float slowCoeff = (targetGain < slowEnvelope) ? opticalAttack : opticalReleaseSlow;
        slowEnvelope = targetGain * slowCoeff + slowEnvelope * (1.f - slowCoeff);

        // Blend: fast mode = mostly fast envelope, normal = blend fast+slow
        float blendFast = fastRelease ? 0.85f : 0.5f;
        float blendedEnv = fastEnvelope * blendFast + slowEnvelope * (1.f - blendFast);
        opticalGain  = juce::jlimit (0.001f, 1.f, blendedEnv);

        // Auto-makeup: subtle compensation like LA-2A internal gain staging
        float autoMakeup = std::pow (opticalGain, -0.07f);
        autoMakeup = juce::jlimit (1.f, 1.15f, autoMakeup);  // cap at ~+1.2dB

        float wetL = applySoftSaturation (inL * opticalGain * makeupGain * autoMakeup);
        float wetR = applySoftSaturation (inR * opticalGain * makeupGain * autoMakeup);

        float outL = inL * (1.f - mixAmount) + wetL * mixAmount;
        float outR = inR * (1.f - mixAmount) + wetR * mixAmount;

        // Output calibration stage (+4 clean / +10 driven by Gain)
        {
            float tRange = outputRange10 ? 1.f : 0.f;
            smoothedDriveRange += (tRange - smoothedDriveRange) * outSmoothCoeff;
            outL = applyOutputStage (outL);
            outR = applyOutputStage (outR);
        }

        if (numChannels > 0) buffer.setSample (0, i, outL);
        if (numChannels > 1) buffer.setSample (1, i, outR);

        float grDb = std::abs (juce::Decibels::gainToDecibels (opticalGain, -60.f));
        if (grDb > grMax) grMax = grDb;
        float outSample = juce::jmax (std::abs (outL), std::abs (outR));
        if (outSample > outPeak) outPeak = outSample;
    }

    currentGR   = grMax;
    outputLevel = juce::Decibels::gainToDecibels (outPeak, -60.f);
}

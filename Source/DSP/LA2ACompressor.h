#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class LA2ACompressor
{
public:
    LA2ACompressor();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    void setPeakReduction (float normalized);
    void setGain (float normalized);
    void setCompressMode (bool compress);
    void setFastMode (bool fast);
    void setHPF   (float freqHz);
    void setEQMid (float freqHz, float gainDb);
    void setEQHi  (float freqHz, float gainDb);
    void setLPF      (float freqHz);
    void setEQBypass (bool bypassed);
    void setMix (float normalized);
    void setStereoLink (bool linked);
    void setTubeMode (bool tube);
    void setOutputRange (bool is10);
    void setCalibration (float offset, float mult, float exp);

    float getGainReductionDb() const { return currentGR; }
    float getOutputLevelDb() const { return outputLevel; }

private:
    double sampleRate = 44100.0;

    float opticalGain = 1.f;
    float fastEnvelope = 1.f;
    float slowEnvelope = 1.f;
    float opticalAttack = 0.f;
    float opticalReleaseFast = 0.f;
    float opticalReleaseSlow = 0.f;
    float currentThreshold = 0.f;
    float currentThresholdGain = 1.f;
    float currentRatio = 1.f;

    float peakReduction = 0.f;
    float makeupGain = 1.f;
    bool compMode = true;
    bool fastRelease = false;
    float mixAmount = 1.f;
    bool stereoLinked = true;
    bool tubeEnabled = true;
    float calOffset = -5.f;
    float calMult = 55.f;
    float calExp = 0.57f;

    float currentGR = 0.f;
    float outputLevel = -60.f;

    // EQ filters (HPF + peak + hi shelf), applied pre-compression
    juce::dsp::IIR::Filter<float> hpfL, hpfR;
    juce::dsp::IIR::Filter<float> midL, midR;
    juce::dsp::IIR::Filter<float> hiL,  hiR;
    juce::dsp::IIR::Filter<float> lpfL, lpfR;

    float eqHpfFreq   = 80.f;
    float eqMidFreq   = 1000.f;
    float eqMidGainDb = 0.f;
    float eqHiFreq    = 8000.f;
    float eqHiGainDb  = 0.f;
    float eqLpfFreq   = 20000.f;
    bool  eqBypassed  = false;

    // Output calibration stage (+4 / +10)
    bool  outputRange10      = false;
    float smoothedDriveRange = 0.f;   // 0 = +4 (clean), 1 = +10 (driven)
    float outSmoothCoeff     = 0.002f;
    float gainNormalized     = 0.f;   // 0..1 from Gain knob

    void updateCoefficients();
    void updateEQCoefficients();

    float applySoftSaturation (float sample);
    float applyOutputStage (float sample);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LA2ACompressor)
};

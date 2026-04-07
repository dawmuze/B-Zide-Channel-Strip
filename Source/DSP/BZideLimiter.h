#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

//==============================================================================
// BZideLimiter — Brickwall limiter for OUTPUT section
//==============================================================================
class BZideLimiter
{
public:
    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr_ = sampleRate;
        envelope_ = 0.0f;
    }

    void setThreshold(float db) { threshold_ = db; }
    void setBypass(bool b) { bypass_ = b; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_) return;

        float threshLin = juce::Decibels::decibelsToGain(threshold_);
        float attackCoeff = std::exp(-1.0f / (float)(sr_ * 0.0001)); // ~0.1ms
        float releaseCoeff = std::exp(-1.0f / (float)(sr_ * 0.050)); // ~50ms

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));

            if (peak > envelope_)
                envelope_ = attackCoeff * envelope_ + (1.0f - attackCoeff) * peak;
            else
                envelope_ = releaseCoeff * envelope_ + (1.0f - releaseCoeff) * peak;

            float gain = 1.0f;
            if (envelope_ > threshLin && threshLin > 0.0f)
                gain = threshLin / envelope_;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
        }
    }

private:
    double sr_ = 44100.0;
    float threshold_ = -0.3f;
    bool bypass_ = true;
    float envelope_ = 0.0f;
};

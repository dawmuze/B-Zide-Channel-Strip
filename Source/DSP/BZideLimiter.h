#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

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

        // Look-ahead delay: ~1ms
        delaySamples_ = std::max(1, (int)(sampleRate * 0.001));
        delayWritePos_ = 0;
        delayBuffer_.clear();
        delayBuffer_.resize(2, std::vector<float>(delaySamples_, 0.0f));
    }

    void setThreshold(float db) { threshold_ = db; }
    void setRelease(float ms) { releaseMs_ = ms; }
    void setBypass(bool b) { bypass_ = b; }
    float getGainReduction() const { return gainReduction_; }
    int getLatencySamples() const { return delaySamples_; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_) { gainReduction_ = 0.0f; return; }

        float threshLin = juce::Decibels::decibelsToGain(threshold_);
        float attackCoeff = std::exp(-1.0f / (float)(sr_ * 0.0001)); // ~0.1ms
        float relSec = releaseMs_ * 0.001f;
        float releaseCoeff = std::exp(-1.0f / (float)(sr_ * (double)relSec));

        float maxGR = 0.0f;

        int numChannels = buffer.getNumChannels();

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Detect peak from CURRENT (non-delayed) sample
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));

            if (peak > envelope_)
                envelope_ = attackCoeff * envelope_ + (1.0f - attackCoeff) * peak;
            else
                envelope_ = releaseCoeff * envelope_ + (1.0f - releaseCoeff) * peak;

            float gain = 1.0f;
            if (envelope_ > threshLin && threshLin > 0.0f)
                gain = threshLin / envelope_;

            float grDb = -juce::Decibels::gainToDecibels(gain);
            if (grDb > maxGR) maxGR = grDb;

            // Apply gain to DELAYED sample (look-ahead)
            for (int ch = 0; ch < numChannels; ++ch)
            {
                int chIdx = std::min(ch, (int)delayBuffer_.size() - 1);
                float delayed = delayBuffer_[chIdx][delayWritePos_];
                delayBuffer_[chIdx][delayWritePos_] = buffer.getSample(ch, i);
                buffer.setSample(ch, i, delayed * gain);
            }
            delayWritePos_ = (delayWritePos_ + 1) % delaySamples_;
        }

        gainReduction_ = maxGR;
    }

private:
    double sr_ = 44100.0;
    float threshold_ = -0.3f;
    float releaseMs_ = 50.0f;
    bool bypass_ = true;
    float envelope_ = 0.0f;
    float gainReduction_ = 0.0f;

    // Look-ahead delay buffer (~1ms)
    std::vector<std::vector<float>> delayBuffer_;
    int delayWritePos_ = 0;
    int delaySamples_ = 48;
};

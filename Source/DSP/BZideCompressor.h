#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <atomic>

//==============================================================================
// BZideCompressor — VCA/FET/OPT compressor for COMP section
//==============================================================================
class BZideCompressor
{
public:
    enum Model { VCA = 0, FET, OPT };

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr_ = sampleRate;
        envelope_ = 0.0f;
    }

    void setModel(Model m) { model_ = m; }
    void setThreshold(float t) { threshold_ = t; }
    void setRatio(float r) { ratio_ = juce::jmax(1.0f, r); }
    void setAttack(float ms) { attackMs_ = juce::jmax(0.1f, ms); }
    void setRelease(float ms) { releaseMs_ = juce::jmax(10.0f, ms); }
    void setMakeupGain(float db) { makeupDb_ = db; }
    void setMix(float pct) { mix_ = juce::jlimit(0.0f, 100.0f, pct) / 100.0f; }
    void setBypass(bool b) { bypass_ = b; }

    float getGainReduction() const { return currentGR_.load(); }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_) { currentGR_.store(0.0f); return; }

        float attackCoeff = std::exp(-1.0f / (float)(sr_ * attackMs_ * 0.001));
        float releaseCoeff = std::exp(-1.0f / (float)(sr_ * releaseMs_ * 0.001));
        float threshLin = juce::Decibels::decibelsToGain(threshold_);
        float makeupLin = juce::Decibels::decibelsToGain(makeupDb_);

        // Model-specific knee and character
        float knee = (model_ == VCA) ? 3.0f : (model_ == FET) ? 1.0f : 6.0f;
        float releaseScale = (model_ == OPT) ? 2.5f : 1.0f; // OPT has slow release

        float peakGR = 0.0f;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Peak detection (stereo linked)
            float peak = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));

            // Envelope follower
            if (peak > envelope_)
                envelope_ = attackCoeff * envelope_ + (1.0f - attackCoeff) * peak;
            else
                envelope_ = (releaseCoeff * releaseScale) * envelope_ + (1.0f - releaseCoeff * releaseScale) * peak;

            // Gain computation
            float gr = 0.0f;
            if (envelope_ > threshLin && threshLin > 0.0f)
            {
                float overDb = juce::Decibels::gainToDecibels(envelope_ / threshLin);
                // Soft knee
                if (overDb < knee)
                    gr = -(overDb * overDb) / (2.0f * knee) * (1.0f - 1.0f / ratio_);
                else
                    gr = -(overDb - knee / 2.0f) * (1.0f - 1.0f / ratio_);
            }

            float gainLin = juce::Decibels::decibelsToGain(gr) * makeupLin;
            peakGR = std::min(peakGR, gr);

            // Apply gain with mix (parallel compression)
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float dry = buffer.getSample(ch, i);
                float wet = dry * gainLin;
                buffer.setSample(ch, i, dry * (1.0f - mix_) + wet * mix_);
            }
        }

        currentGR_.store(peakGR);
    }

private:
    double sr_ = 44100.0;
    Model model_ = VCA;
    float threshold_ = -20.0f;
    float ratio_ = 4.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 100.0f;
    float makeupDb_ = 0.0f;
    float mix_ = 1.0f;
    bool bypass_ = true;
    float envelope_ = 0.0f;
    std::atomic<float> currentGR_ { 0.0f };
};

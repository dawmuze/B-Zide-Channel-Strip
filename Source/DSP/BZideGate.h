#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

//==============================================================================
// BZideGate — Gate/Expander for GATE section
//==============================================================================
class BZideGate
{
public:
    enum Mode { GATE = 0, EXPANDER };

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr_ = sampleRate;
        envelope_ = 0.0f;
    }

    void setMode(Mode m) { mode_ = m; }
    void setThreshold(float db) { threshold_ = db; }
    void setAttenuation(float db) { atten_ = db; }
    void setFloor(float db) { floor_ = db; }
    void setAttack(float ms) { attackMs_ = juce::jmax(0.1f, ms); }
    void setRelease(float ms) { releaseMs_ = juce::jmax(10.0f, ms); }
    void setBypass(bool b) { bypass_ = b; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_) return;

        float attackCoeff = std::exp(-1.0f / (float)(sr_ * attackMs_ * 0.001));
        float releaseCoeff = std::exp(-1.0f / (float)(sr_ * releaseMs_ * 0.001));
        float threshLin = juce::Decibels::decibelsToGain(threshold_);
        float floorLin = juce::Decibels::decibelsToGain(floor_);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Peak detection
            float peak = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));

            // Envelope
            if (peak > envelope_)
                envelope_ = attackCoeff * envelope_ + (1.0f - attackCoeff) * peak;
            else
                envelope_ = releaseCoeff * envelope_ + (1.0f - releaseCoeff) * peak;

            // Gate gain
            float gain = 1.0f;
            if (envelope_ < threshLin)
            {
                if (mode_ == GATE)
                    gain = floorLin;
                else // Expander: proportional reduction
                    gain = std::max(floorLin, envelope_ / threshLin);
            }

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
        }
    }

private:
    double sr_ = 44100.0;
    Mode mode_ = GATE;
    float threshold_ = -40.0f;
    float atten_ = -30.0f;
    float floor_ = -60.0f;
    float attackMs_ = 1.0f;
    float releaseMs_ = 200.0f;
    bool bypass_ = true;
    float envelope_ = 0.0f;
};

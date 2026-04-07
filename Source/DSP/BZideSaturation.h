#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

//==============================================================================
// BZideSaturation — ODD/EVEN/HEAVY saturation modes for PRE section
//==============================================================================
class BZideSaturation
{
public:
    enum Mode { ODD = 0, EVEN, HEAVY };

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr_ = sampleRate;
        dcBlockCoeff_ = 1.0f - (float)(2.0 * 3.14159265 * 5.0 / sampleRate); // ~5Hz HPF
    }

    void setMode(Mode m) { mode_ = m; }
    void setDrive(float d) { drive_ = juce::jlimit(0.0f, 100.0f, d); }
    void setTone(float t) { tone_ = juce::jlimit(-100.0f, 100.0f, t); }
    void setBypass(bool b) { bypass_ = b; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_ || drive_ < 0.1f) return;

        float driveGain = 1.0f + drive_ * 0.1f; // 1x to 11x
        float toneBlend = (tone_ + 100.0f) / 200.0f; // 0..1

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float x = data[i] * driveGain;
                float y = 0.0f;

                switch (mode_)
                {
                    case ODD:   y = std::tanh(x); break;
                    case EVEN:  y = (x >= 0) ? (1.0f - std::exp(-x)) : -(1.0f - std::exp(x)); y = (y + std::tanh(x * 0.5f)) * 0.5f; break;
                    case HEAVY: y = std::tanh(x * 2.0f) * 0.7f + std::atan(x * 3.0f) * 0.15f; break;
                }

                // Tone: blend between dark (LPF) and bright
                y = y * toneBlend + y * (1.0f - toneBlend) * 0.8f;

                // DC blocker
                float dcOut = y - dcX_[ch] + dcBlockCoeff_ * dcY_[ch];
                dcX_[ch] = y;
                dcY_[ch] = dcOut;

                data[i] = dcOut / driveGain; // Compensate gain
            }
        }
    }

private:
    double sr_ = 44100.0;
    Mode mode_ = ODD;
    float drive_ = 0.0f;
    float tone_ = 0.0f;
    bool bypass_ = true;
    float dcBlockCoeff_ = 0.995f;
    float dcX_[2] = {}, dcY_[2] = {};
};

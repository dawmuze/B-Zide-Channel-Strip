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

    void prepare(double sampleRate, int samplesPerBlock)
    {
        sr_ = sampleRate;
        dcBlockCoeff_ = 1.0f - (float)(2.0 * 3.14159265 * 5.0 / sampleRate); // ~5Hz HPF

        // 2x oversampling (order 1 = 2x, polyphase IIR)
        oversampling_ = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
        oversampling_->initProcessing(static_cast<size_t>(samplesPerBlock));

        // Tone filters
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 10000.0f);
        *toneFilterL_.coefficients = *coeffs;
        *toneFilterR_.coefficients = *coeffs;
        toneFilterL_.reset();
        toneFilterR_.reset();
    }

    void setMode(Mode m) { mode_ = m; }
    void setDrive(float d) { drive_ = juce::jlimit(0.0f, 100.0f, d); }
    void setTone(float t) { tone_ = juce::jlimit(-100.0f, 100.0f, t); }
    void setBypass(bool b) { bypass_ = b; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_ || drive_ < 0.1f) return;

        float driveGain = 1.0f + drive_ * 0.1f; // 1x to 11x

        // Update tone filter cutoff: tone=-100 -> 2000Hz, tone=+100 -> 18000Hz
        float cutoff = 2000.0f + (tone_ + 100.0f) * 80.0f;
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr_, cutoff);
        *toneFilterL_.coefficients = *coeffs;
        *toneFilterR_.coefficients = *coeffs;

        // Upsample
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto oversampledBlock = oversampling_->processSamplesUp(block);

        int numOsSamples = static_cast<int>(oversampledBlock.getNumSamples());
        int numChannels = static_cast<int>(oversampledBlock.getNumChannels());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
            for (int i = 0; i < numOsSamples; ++i)
            {
                float x = data[i] * driveGain;
                float y = 0.0f;

                switch (mode_)
                {
                    case ODD:   y = std::tanh(x); break;
                    case EVEN:  y = (x >= 0) ? (1.0f - std::exp(-x)) : -(1.0f - std::exp(x)); y = (y + std::tanh(x * 0.5f)) * 0.5f; break;
                    case HEAVY: y = std::tanh(x * 2.0f) * 0.7f + std::atan(x * 3.0f) * 0.15f; break;
                }

                data[i] = y / driveGain; // Compensate gain
            }
        }

        // Downsample
        oversampling_->processSamplesDown(block);

        // Tone filter + DC blocker at original sample rate
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            auto& toneFilter = (ch == 0) ? toneFilterL_ : toneFilterR_;
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float y = toneFilter.processSample(data[i]);

                // DC blocker
                float dcOut = y - dcX_[ch] + dcBlockCoeff_ * dcY_[ch];
                dcX_[ch] = y;
                dcY_[ch] = dcOut;

                data[i] = dcOut;
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

    // 2x oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling_;

    // Tone filters (proper LPF instead of crude blend)
    juce::dsp::IIR::Filter<float> toneFilterL_, toneFilterR_;
};

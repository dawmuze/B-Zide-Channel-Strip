#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

//==============================================================================
// BZideSaturation — ODD/EVEN/HEAVY saturation modes for PRE section
// 2x oversampling (ODD/EVEN), 4x oversampling (HEAVY) for alias-free harmonics
//==============================================================================
class BZideSaturation
{
public:
    enum Mode { ODD = 0, EVEN, HEAVY, CRUNCH };

    void prepare(double sampleRate, int samplesPerBlock)
    {
        sr_ = sampleRate;
        // DC blocker at oversampled rate needs different coefficient
        // Will be recalculated when oversampling is set up

        // Create oversampling based on current mode
        rebuildOversampling(samplesPerBlock);

        // Tone filters (at original sample rate — tonal shaping, not anti-aliasing)
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 10000.0f);
        *toneFilterL_.coefficients = *coeffs;
        *toneFilterR_.coefficients = *coeffs;
        toneFilterL_.reset();
        toneFilterR_.reset();
        cachedToneCutoff_ = -1.0f;

        lastBlockSize_ = samplesPerBlock;
    }

    void setMode(Mode m)
    {
        if (m != mode_)
        {
            mode_ = m;
            // Rebuild oversampling if switching to/from HEAVY (4x vs 2x)
            if (lastBlockSize_ > 0)
                rebuildOversampling(lastBlockSize_);
        }
    }

    void setDrive(float d) { drive_ = juce::jlimit(0.0f, 100.0f, d); }
    void setTone(float t) { tone_ = juce::jlimit(-100.0f, 100.0f, t); }
    void setBypass(bool b) { bypass_ = b; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!oversampling_) return;

        // Always process through oversampling for latency consistency (even on bypass)
        if (bypass_ || drive_ < 0.1f)
        {
            // Pass through oversampling (up + down) to maintain latency, no saturation
            auto block = juce::dsp::AudioBlock<float>(buffer);
            auto osBlock = oversampling_->processSamplesUp(block);
            oversampling_->processSamplesDown(block);
            return;
        }

        float driveGain = 1.0f + drive_ * 0.1f; // 1x to 11x

        // Update tone filter cutoff (at original rate)
        float cutoff = 2000.0f + (tone_ + 100.0f) * 80.0f;
        if (std::abs(cutoff - cachedToneCutoff_) > 1.0f)
        {
            cachedToneCutoff_ = cutoff;
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr_, cutoff);
            *toneFilterL_.coefficients = *coeffs;
            *toneFilterR_.coefficients = *coeffs;
        }

        // Upsample (2x or 4x depending on mode)
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto oversampledBlock = oversampling_->processSamplesUp(block);

        int numOsSamples = static_cast<int>(oversampledBlock.getNumSamples());
        int numChannels = static_cast<int>(oversampledBlock.getNumChannels());

        // Saturation + DC blocker at oversampled rate
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
            for (int i = 0; i < numOsSamples; ++i)
            {
                float x = data[i] * driveGain;
                float y = 0.0f;

                switch (mode_)
                {
                    case ODD:    y = std::tanh(x); break;
                    case EVEN:   y = (x >= 0) ? (1.0f - std::exp(-x)) : -(1.0f - std::exp(x));
                                 y = (y + std::tanh(x * 0.5f)) * 0.5f; break;
                    case HEAVY:  y = std::tanh(x * 2.0f) * 0.7f + std::atan(x * 3.0f) * 0.15f; break;
                    case CRUNCH: y = std::tanh(x * 1.5f) * 0.8f + std::tanh(x * 0.5f) * 0.2f; break;
                }

                y /= driveGain; // Compensate gain

                // DC blocker at oversampled rate (catches DC before downsample)
                float dcOut = y - dcX_[ch] + dcBlockCoeff_ * dcY_[ch];
                dcX_[ch] = y;
                dcY_[ch] = dcOut;

                data[i] = dcOut;
            }
        }

        // Downsample (anti-aliasing filter applied by JUCE)
        oversampling_->processSamplesDown(block);

        // Tone filter at original sample rate (tonal shaping, not anti-aliasing)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            auto& toneFilter = (ch == 0) ? toneFilterL_ : toneFilterR_;
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                data[i] = toneFilter.processSample(data[i]);
        }
    }

    int getLatencySamples() const { return oversampling_ ? (int)oversampling_->getLatencyInSamples() : 0; }

private:
    void rebuildOversampling(int samplesPerBlock)
    {
        // HEAVY/CRUNCH use 4x oversampling (order 2 = 2^2), ODD/EVEN use 2x (order 1 = 2^1)
        int order = (mode_ == HEAVY || mode_ == CRUNCH) ? 2 : 1;
        if (currentOsOrder_ != order)
        {
            oversampling_ = std::make_unique<juce::dsp::Oversampling<float>>(
                2, order, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
            oversampling_->initProcessing(static_cast<size_t>(samplesPerBlock));
            currentOsOrder_ = order;

            // Recalculate DC blocker coefficient at oversampled rate
            double osRate = sr_ * (1 << order);
            dcBlockCoeff_ = 1.0f - (float)(2.0 * 3.14159265 * 5.0 / osRate);
        }
    }

    double sr_ = 44100.0;
    Mode mode_ = ODD;
    float drive_ = 0.0f;
    float tone_ = 0.0f;
    float cachedToneCutoff_ = -1.0f;
    bool bypass_ = true;
    float dcBlockCoeff_ = 0.995f;
    float dcX_[2] = {}, dcY_[2] = {};
    int currentOsOrder_ = -1;
    int lastBlockSize_ = 0;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling_;
    juce::dsp::IIR::Filter<float> toneFilterL_, toneFilterR_;
};

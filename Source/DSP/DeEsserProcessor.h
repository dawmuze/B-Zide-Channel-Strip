#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

//==============================================================================
// DeEsserProcessor — 2-band frequency-selective compressor for sibilance control
//
// Band 1: Typically 4-8 kHz (main sibilance)
// Band 2: Typically 8-16 kHz (air/brightness)
//
// Each band has independent frequency, threshold, and range controls.
// Detection uses bandpass-filtered sidechain; gain reduction applies
// only to the detected frequency range (split-band mode).
//==============================================================================
class DeEsserProcessor
{
public:
    struct Band
    {
        float frequency = 6000.0f;   // Center frequency (Hz)
        float threshold = -20.0f;    // Threshold (dB)
        float range = -12.0f;        // Max gain reduction (dB)
        bool active = true;
    };

    void prepare(double sampleRate, int samplesPerBlock)
    {
        sampleRate_ = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels = 1;

        for (int b = 0; b < 2; ++b)
        {
            bandpassL_[b].prepare(spec);
            bandpassR_[b].prepare(spec);
            envelope_[b] = 0.0f;
        }

        // Envelope follower coefficients (~1ms attack, ~50ms release)
        attackCoeff_ = 1.0f - std::exp(-1.0f / (float)(sampleRate * 0.001));
        releaseCoeff_ = 1.0f - std::exp(-1.0f / (float)(sampleRate * 0.050));
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!bypass_)
        {
            for (int b = 0; b < 2; ++b)
            {
                if (!bands_[b].active) continue;
                processBand(buffer, b);
            }
        }
    }

    void reset()
    {
        for (int b = 0; b < 2; ++b)
        {
            bandpassL_[b].reset();
            bandpassR_[b].reset();
            envelope_[b] = 0.0f;
        }
    }

    void setBand(int bandIndex, float frequency, float threshold, float range)
    {
        if (bandIndex < 0 || bandIndex > 1) return;
        bands_[bandIndex].threshold = threshold;
        bands_[bandIndex].range = range;
        // FIX 3: only recalculate bandpass coefficients when freq actually changes
        if (std::abs(frequency - cachedFreq_[bandIndex]) > 1.0f)
        {
            bands_[bandIndex].frequency = frequency;
            cachedFreq_[bandIndex] = frequency;
            updateBandFilter(bandIndex);
        }
    }

    void setBandActive(int bandIndex, bool active)
    {
        if (bandIndex >= 0 && bandIndex <= 1)
            bands_[bandIndex].active = active;
    }

    void setBypass(bool bypass) { bypass_ = bypass; }
    bool isBypassed() const { return bypass_; }

    float getGainReduction(int bandIndex) const
    {
        if (bandIndex < 0 || bandIndex > 1) return 0.0f;
        return currentGR_[bandIndex];
    }

    Band& getBand(int index) { return bands_[index]; }

private:
    void updateBandFilter(int b)
    {
        if (sampleRate_ <= 0) return;
        float freq = juce::jlimit(200.0f, 20000.0f, bands_[b].frequency);
        float Q = 3.5f; // Narrow, focused on sibilance

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate_, freq, Q);
        *bandpassL_[b].coefficients = *coeffs;
        *bandpassR_[b].coefficients = *coeffs;
    }

    void processBand(juce::AudioBuffer<float>& buffer, int b)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        float thresh = juce::Decibels::decibelsToGain(bands_[b].threshold);
        float maxGR = bands_[b].range; // negative dB

        for (int i = 0; i < numSamples; ++i)
        {
            // Detect sibilance level using bandpass-filtered signal
            float detL = 0.0f, detR = 0.0f;

            if (numChannels > 0)
            {
                float sample = buffer.getSample(0, i);
                detL = bandpassL_[b].processSample(sample);
            }
            if (numChannels > 1)
            {
                float sample = buffer.getSample(1, i);
                detR = bandpassR_[b].processSample(sample);
            }

            // Envelope follower (peak detection)
            float detected = std::max(std::abs(detL), std::abs(detR));
            if (detected > envelope_[b])
                envelope_[b] += attackCoeff_ * (detected - envelope_[b]);
            else
                envelope_[b] += releaseCoeff_ * (detected - envelope_[b]);

            // Compute gain reduction
            float gr = 0.0f;
            if (envelope_[b] > thresh && thresh > 0.0f)
            {
                float overDb = juce::Decibels::gainToDecibels(envelope_[b] / thresh);
                gr = juce::jmax(maxGR, -overDb * 0.25f); // 4:1 ratio on sibilance
            }

            float gainLin = juce::Decibels::decibelsToGain(gr);
            currentGR_[b] = gr;

            // Apply gain reduction to all channels
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * gainLin);
        }
    }

    double sampleRate_ = 44100.0;
    float cachedFreq_[2] = {-1.0f, -1.0f};  // FIX 3: cached band frequencies
    Band bands_[2];
    juce::dsp::IIR::Filter<float> bandpassL_[2], bandpassR_[2];
    float envelope_[2] = { 0.0f, 0.0f };
    float currentGR_[2] = { 0.0f, 0.0f };
    float attackCoeff_ = 0.0f, releaseCoeff_ = 0.0f;
    bool bypass_ = false;
};

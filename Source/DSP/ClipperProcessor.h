#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

//==============================================================================
// ClipperProcessor — Soft/Hard clipper with oversampling and waveform display.
// Processes after all other effects, right before output gain.
//==============================================================================
class ClipperProcessor
{
public:
    ClipperProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 bool forceProcess = false);
    void reset();

    // ─── Waveform ring buffer for UI display ────────────────────────────
    static constexpr int waveformSize = 65536;  // ~1.5 sec at 44.1kHz
    std::array<float, waveformSize> waveformBuffer {};  // post-clip mono signal
    std::atomic<int> waveformWritePos { 0 };

    // ─── Auto-threshold state (read by UI) ──────────────────────────────
    std::atomic<float> currentAutoThreshDb { -6.0f };
    std::atomic<bool>  autoThreshEnabled { false };

    // ─── Current threshold in dB for display ────────────────────────────
    std::atomic<float> displayThreshDb { -6.0f };

private:
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    bool prepared = false;

    // ─── Dry buffer for mix ─────────────────────────────────────────────
    juce::AudioBuffer<float> dryBuffer;

    // ─── Oversampling (2x) ──────────────────────────────────────────────
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // ─── Auto-threshold peak tracker ────────────────────────────────────
    float peakTracker_ = 0.0f;

    // ─── Clipping functions ─────────────────────────────────────────────
    static float hardClip(float sample, float threshold);
    static float softClip(float sample, float threshold);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipperProcessor)
};

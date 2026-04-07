#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class GateProcessor
{
public:
    enum Model { Hard = 0, Soft, Expander, NumModels };

    GateProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 bool forceProcess = false);
    void reset();

    // --- External sidechain support ---
    // When set, the gate uses this signal for level detection instead of the input audio.
    void setExternalSidechain(const juce::AudioBuffer<float>* scBuffer) { externalSidechain_ = scBuffer; }

    // --- Sidechain bandpass filter control ---
    // Configure the frequency range for sidechain detection.
    // lowHz: high-pass cutoff (clamped >= 20 Hz)
    // highHz: low-pass cutoff (clamped <= 20000 Hz, must be > lowHz)
    void setSidechainFilter(float lowHz, float highHz);
    void setSidechainFilterEnabled(bool enabled);

    // --- Gate activity metering (atomic, read by UI) ---
    // 0.0 = gate fully open (no attenuation), negative = gate active (e.g. -40 = 40dB reduction)
    std::atomic<float> currentGateReductionDb { 0.0f };

private:
    // --- External sidechain (one-block latency from source track) ---
    const juce::AudioBuffer<float>* externalSidechain_ = nullptr;
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    bool prepared = false;

    // --- Envelope followers (per channel) ---
    float envelopeL = 0.0f;
    float envelopeR = 0.0f;

    // --- Smoothed gate gain (anti-zipper) ---
    float smoothedGateL = 1.0f;
    float smoothedGateR = 1.0f;

    // --- Soft model: program-dependent hysteresis state ---
    float hysteresisL = 0.0f;
    float hysteresisR = 0.0f;

    // --- Sidechain bandpass filter (HPF + LPF in series on detection path) ---
    float sidechainLowFreq  = 20.0f;      // High-pass cutoff (Hz)
    float sidechainHighFreq = 20000.0f;    // Low-pass cutoff (Hz)
    bool  sidechainFilterEnabled = false;

    juce::dsp::IIR::Filter<float> sidechainHPF_L, sidechainHPF_R;
    juce::dsp::IIR::Filter<float> sidechainLPF_L, sidechainLPF_R;

    // --- Lookahead buffer ---
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadSamples = 0;
    int lookaheadWritePos = 0;

    // --- Per-model gate gain computation ---
    float computeGateHard(float inputLevel, float thresholdLin,
                          float attackSec, float releaseSec, float& envelope);
    float computeGateSoft(float inputLevel, float thresholdLin,
                          float attackSec, float releaseSec,
                          float& envelope, float& hysteresis);
    float computeGateExpander(float inputLevel, float thresholdLin,
                              float ratio, float attackSec, float releaseSec,
                              float& envelope);

    // --- Filter setup ---
    void updateSidechainFilter();
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class CompressorProcessor
{
public:
    enum Model { VCA = 0, Opto, FET, VariMu, NumModels };

    CompressorProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 bool forceProcess = false);
    void reset();

    // --- External sidechain support ---
    // Set an external sidechain buffer (from another track's previous block output).
    // When set, the compressor uses this signal for level detection instead of the input audio.
    // Pass nullptr to revert to internal sidechain (default).
    void setExternalSidechain(const juce::AudioBuffer<float>* scBuffer) { externalSidechain_ = scBuffer; }

    // --- Gain Reduction metering (atomic, read by UI) ---
    std::atomic<float> currentGainReductionDb { 0.0f };  // negative dB (e.g. -6.0 = 6dB GR)

private:
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    bool prepared = false;

    // --- Envelope followers (per channel) ---
    float envelopeL = 0.0f;
    float envelopeR = 0.0f;

    // --- Smoothed gain reduction (anti-zipper) ---
    float smoothedGainL = 1.0f;
    float smoothedGainR = 1.0f;

    // --- Opto model: LDR emulation state ---
    float optoAttenuationL = 0.0f;
    float optoAttenuationR = 0.0f;
    float optoPeakL = 0.0f;
    float optoPeakR = 0.0f;

    // --- VariMu model: tube bias envelope ---
    float tubeEnvelopeL = 0.0f;
    float tubeEnvelopeR = 0.0f;

    // --- Sidechain HPF (removes lows from detector, prevents pumping) ---
    juce::dsp::IIR::Filter<float> sidechainHPF_L, sidechainHPF_R;

    // --- FET model: output transformer coloration ---
    juce::dsp::IIR::Filter<float> fetColorFilterL, fetColorFilterR;

    // --- VariMu model: tube warmth filter ---
    juce::dsp::IIR::Filter<float> tubeWarmthFilterL, tubeWarmthFilterR;

    // --- Pre-allocated dry buffer for parallel compression ---
    juce::AudioBuffer<float> dryBuffer;

    // --- External sidechain (one-block latency from source track) ---
    const juce::AudioBuffer<float>* externalSidechain_ = nullptr;

    // --- Per-model gain computation ---
    float computeGainVCA(float inputLevel, float threshold, float ratio,
                         float attackSec, float releaseSec, float& envelope);
    float computeGainOpto(float inputLevel, float threshold, float ratio,
                          float attackSec, float releaseSec,
                          float& envelope, float& optoAtten, float& optoPeak);
    float computeGainFET(float inputLevel, float threshold, float ratio,
                         float attackSec, float releaseSec, float& envelope);
    float computeGainVariMu(float inputLevel, float threshold, float ratio,
                            float attackSec, float releaseSec,
                            float& envelope, float& tubeEnv);

    // --- Model-specific waveshapers ---
    static float fetSaturate(float sample, float amount);
    static float tubeWarmth(float sample, float amount);

    // --- Filter setup ---
    void updateSidechainFilter();
    void updateFETColorFilter();
    void updateTubeWarmthFilter();
};

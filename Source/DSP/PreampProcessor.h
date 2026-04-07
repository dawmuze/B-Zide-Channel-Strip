#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class PreampProcessor
{
public:
    enum Model { Clean = 0, Neve1073, SSLESeries, API512, TapeStuder, NumModels };

    PreampProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 bool forceProcess = false);
    void reset();

private:
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    bool prepared = false;  // Guard against process() before prepare()

    // Tone shaping filters (stereo)
    juce::dsp::IIR::Filter<float> toneFilterL, toneFilterR;

    // Tape model: head bump + HF rolloff
    juce::dsp::IIR::Filter<float> headBumpL, headBumpR;
    juce::dsp::IIR::Filter<float> hfRolloffL, hfRolloffR;

    // Tape compression envelope followers
    float tapeEnvelopeL = 0.0f;
    float tapeEnvelopeR = 0.0f;

    // Pre-allocated dry buffer for mix blending (avoids allocation in audio thread)
    juce::AudioBuffer<float> dryBuffer;

    // 4× oversampling (factor = 2 → 2^2 = 4×)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // DC blocker state (1-pole HPF ~5 Hz): y[n] = x[n] - x[n-1] + R * y[n-1]
    float dcX1[2] = {0.0f, 0.0f};
    float dcY1[2] = {0.0f, 0.0f};
    float dcBlockerR = 0.995f;  // coefficient recalculated in prepare()

    // Waveshaping per model
    static float processClean(float sample, float drive);
    static float processNeve(float sample, float drive);
    static float processSSL(float sample, float drive);
    static float processAPI(float sample, float drive);
    float processTapeSample(float sample, float drive, float& envelope);

    void updateToneFilter(Model model, float tonePercent);
    void updateTapeFilters(float tonePercent);

    // Change detection — avoid heap allocation when params haven't changed
    int   lastFilterModel_ = -1;
    float lastFilterTone_  = -999.0f;
};

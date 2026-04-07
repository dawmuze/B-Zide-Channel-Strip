#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SaturationProcessor
{
public:
    enum Model { Tape = 0, Tube, Transistor, Digital, NumModels };

    SaturationProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 bool forceProcess = false);
    void reset();

private:
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    bool prepared = false;

    // --- Pre-allocated dry buffer for mix ---
    juce::AudioBuffer<float> dryBuffer;

    // --- Tape model: bias + head bump filter ---
    juce::dsp::IIR::Filter<float> tapeHeadBumpL, tapeHeadBumpR;
    juce::dsp::IIR::Filter<float> tapeLossFilterL, tapeLossFilterR;

    // --- Tube model: output transformer resonance ---
    juce::dsp::IIR::Filter<float> tubeResonanceL, tubeResonanceR;

    // --- Transistor model: presence boost ---
    juce::dsp::IIR::Filter<float> transistorPresL, transistorPresR;

    // --- Per-model saturation functions ---
    static float tapeSaturate(float sample, float drive);
    static float tubeSaturate(float sample, float drive);
    static float transistorSaturate(float sample, float drive);
    static float digitalSaturate(float sample, float drive, float sampleRate);

    // --- Filter setup ---
    void updateTapeFilters();
    void updateTubeFilters();
    void updateTransistorFilters();

    // --- Digital model state ---
    float digitalPhaseL = 0.0f;
    float digitalPhaseR = 0.0f;

    // --- 4× oversampling ---
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // --- DC blocker (1-pole HPF ~5Hz) ---
    float dcX1[2] = {};
    float dcY1[2] = {};
    float dcBlockerR = 0.995f;
};

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <atomic>

//==============================================================================
// BZideCompressor — VCA/FET/OPT compressor for COMP section
//==============================================================================
class BZideCompressor
{
public:
    enum Model { VCA = 0, FET, OPT };
    enum DetectMode { RMS = 0, PEAK = 1 };
    enum Topology { FF = 0, FB = 1 };

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr_ = sampleRate;
        envelope_ = 0.0f;
        lastGainLin_ = 1.0f;
        rmsSum_ = 0.0f;
        smoothedGain_ = 1.0f;
        rmsEnv_ = 0.0f;

        // Prepare SC HPF filters (100 Hz high-pass)
        auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 100.0f);
        scHpfL_.coefficients = hpfCoeffs;
        scHpfR_.coefficients = hpfCoeffs;
        scHpfL_.reset();
        scHpfR_.reset();
    }

    void setModel(Model m) { model_ = m; }
    void setThreshold(float t) { threshold_ = t; }
    void setRatio(float r) { ratio_ = juce::jmax(1.0f, r); }
    void setAttack(float ms) { attackMs_ = juce::jmax(0.1f, ms); }
    void setRelease(float ms) { releaseMs_ = juce::jmax(10.0f, ms); }
    void setMakeupGain(float db) { makeupDb_ = db; }
    void setMix(float pct) { mix_ = juce::jlimit(0.0f, 100.0f, pct) / 100.0f; }
    void setBypass(bool b) { bypass_ = b; }
    void setDetectMode(DetectMode m) { detectMode_ = m; }
    void setTopology(Topology t) { topology_ = t; }
    void setScHpfEnabled(bool e) { scHpfEnabled_ = e; }

    float getGainReduction() const { return currentGR_.load(); }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass_) { currentGR_.store(0.0f); return; }

        float attackCoeff = std::exp(-1.0f / (float)(sr_ * attackMs_ * 0.001));
        float releaseCoeff = std::exp(-1.0f / (float)(sr_ * releaseMs_ * 0.001));
        float threshLin = juce::Decibels::decibelsToGain(threshold_);
        float makeupLin = juce::Decibels::decibelsToGain(makeupDb_);

        // Model-specific knee and character
        float knee = (model_ == VCA) ? 3.0f : (model_ == FET) ? 1.0f : 6.0f;
        float releaseScale = (model_ == OPT) ? 2.5f : 1.0f; // OPT has slow release

        float peakGR = 0.0f;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Detection signal — stereo linked
            float detL = buffer.getSample(0, i);
            float detR = buffer.getNumChannels() > 1 ? buffer.getSample(1, i) : detL;

            // SC HPF — filter the detection signal
            if (scHpfEnabled_)
            {
                detL = scHpfL_.processSample(detL);
                detR = scHpfR_.processSample(detR);
            }

            float level = 0.0f;
            if (detectMode_ == PEAK)
            {
                // Peak detection
                level = std::max(std::abs(detL), std::abs(detR));
            }
            else
            {
                // RMS detection — proper envelope with ~10ms window
                float peak = std::max(std::abs(detL), std::abs(detR));
                float rmsCoeff = std::exp(-1.0f / (float)(sr_ * 0.010));
                rmsEnv_ = rmsCoeff * rmsEnv_ + (1.0f - rmsCoeff) * (peak * peak);
                level = std::sqrt(rmsEnv_);
            }

            // Feed-back topology: multiply detected level by last gain reduction
            if (topology_ == FB)
                level *= lastGainLin_;

            // Envelope follower
            if (level > envelope_)
                envelope_ = attackCoeff * envelope_ + (1.0f - attackCoeff) * level;
            else
                envelope_ = (releaseCoeff * releaseScale) * envelope_ + (1.0f - releaseCoeff * releaseScale) * level;

            // For RMS mode, take sqrt of the envelope for level comparison
            float envLevel = (detectMode_ == RMS) ? std::sqrt(envelope_) : envelope_;

            // Gain computation
            float gr = 0.0f;
            if (envLevel > threshLin && threshLin > 0.0f)
            {
                float overDb = juce::Decibels::gainToDecibels(envLevel / threshLin);
                // Soft knee
                if (overDb < knee)
                    gr = -(overDb * overDb) / (2.0f * knee) * (1.0f - 1.0f / ratio_);
                else
                    gr = -(overDb - knee / 2.0f) * (1.0f - 1.0f / ratio_);
            }

            float gainLin = juce::Decibels::decibelsToGain(gr) * makeupLin;
            lastGainLin_ = juce::Decibels::decibelsToGain(gr); // store raw GR for FB
            lastGainLin_ = juce::jmax(0.01f, lastGainLin_); // Phase 1D: FB stability clamp
            peakGR = std::min(peakGR, gr);

            // Phase 1B: Anti-zipper smoothing (~2ms at 44.1kHz)
            float smoothCoeff = 0.9995f;
            smoothedGain_ = smoothCoeff * smoothedGain_ + (1.0f - smoothCoeff) * gainLin;

            // Apply gain with mix (parallel compression)
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float dry = buffer.getSample(ch, i);
                float wet = dry * smoothedGain_;
                // Phase 4B: FET character — subtle tanh saturation
                if (model_ == FET)
                    wet = std::tanh(wet * 1.1f) * 0.909f;
                buffer.setSample(ch, i, dry * (1.0f - mix_) + wet * mix_);
            }
        }

        currentGR_.store(peakGR);
    }

private:
    double sr_ = 44100.0;
    Model model_ = VCA;
    DetectMode detectMode_ = RMS;
    Topology topology_ = FF;
    bool scHpfEnabled_ = false;
    float threshold_ = -20.0f;
    float ratio_ = 4.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 100.0f;
    float makeupDb_ = 0.0f;
    float mix_ = 1.0f;
    bool bypass_ = true;
    float envelope_ = 0.0f;
    float lastGainLin_ = 1.0f;
    float rmsSum_ = 0.0f;
    float smoothedGain_ = 1.0f;   // Phase 1B: anti-zipper
    float rmsEnv_ = 0.0f;         // Phase 1C: proper RMS envelope
    std::atomic<float> currentGR_ { 0.0f };

    // Sidechain HPF filters (one per channel)
    juce::dsp::IIR::Filter<float> scHpfL_, scHpfR_;
};

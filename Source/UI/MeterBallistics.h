#pragma once
#include <juce_core/juce_core.h>

enum class MeterMode
{
    Peak,
    RMS,
    EBU_R128,
    VU,
    PPM_BBC,
    PPM_EBU
};

class MeterBallistics
{
public:
    MeterBallistics();

    void setMode (MeterMode newMode);
    MeterMode getMode() const { return mode; }

    void process (float inputDb, double elapsedMs);
    void reset();

    float getCurrentDb() const { return currentDb; }
    float getPeakDb() const { return peakDb; }
    bool isOverloaded (float olThreshold) const { return peakDb >= olThreshold; }

    void setHoldTimeMs (float ms) { holdTimeMs = ms; }
    void setReferenceLevel (float dB) { referenceLevel = dB; }
    float getReferenceLevel() const { return referenceLevel; }

    static juce::String getModeName (MeterMode m);

private:
    MeterMode mode = MeterMode::VU;

    float currentDb = 0.f;
    float peakDb = 0.f;
    float velocity = 0.f; // for spring-damper (VU mode)

    float holdTimeMs = 2000.f;
    float holdCounter = 0.f;
    float referenceLevel = 0.f;

    // ballistics coefficients
    float attackMs = 300.f;
    float releaseMs = 300.f;

    void updateCoefficients();
};

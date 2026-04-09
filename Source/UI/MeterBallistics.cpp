#include "MeterBallistics.h"
#include <cmath>

MeterBallistics::MeterBallistics()
{
    updateCoefficients();
}

void MeterBallistics::setMode (MeterMode newMode)
{
    mode = newMode;
    updateCoefficients();
    reset();
}

void MeterBallistics::reset()
{
    currentDb = 0.f;
    peakDb = 0.f;
    velocity = 0.f;
    holdCounter = 0.f;
}

void MeterBallistics::updateCoefficients()
{
    switch (mode)
    {
        case MeterMode::Peak:
            attackMs = 0.1f;
            releaseMs = 500.f;
            break;
        case MeterMode::RMS:
            attackMs = 300.f;
            releaseMs = 300.f;
            break;
        case MeterMode::EBU_R128:
            attackMs = 400.f;
            releaseMs = 400.f;
            break;
        case MeterMode::VU:
            attackMs = 300.f;
            releaseMs = 300.f;
            break;
        case MeterMode::PPM_BBC:
            attackMs = 5.f;
            releaseMs = 1700.f;
            break;
        case MeterMode::PPM_EBU:
            attackMs = 10.f;
            releaseMs = 1700.f;
            break;
    }
}

void MeterBallistics::process (float inputDb, double elapsedMs)
{
    float targetDb = inputDb + referenceLevel;

    if (mode == MeterMode::VU)
    {
        // Standard VU: symmetric 300ms attack AND release (IEC 268-10)
        float dt = static_cast<float> (elapsedMs);
        float coeff = std::exp (-dt / 300.f); // 300ms in both directions
        currentDb = targetDb + coeff * (currentDb - targetDb);
        velocity = 0.f;
    }
    else
    {
        // Simple envelope follower for other modes
        float dt = static_cast<float> (elapsedMs);

        if (targetDb > currentDb)
        {
            float coeff = (attackMs > 0.f) ? std::exp (-dt / attackMs) : 0.f;
            currentDb = targetDb + coeff * (currentDb - targetDb);
        }
        else
        {
            float coeff = (releaseMs > 0.f) ? std::exp (-dt / releaseMs) : 0.f;
            currentDb = targetDb + coeff * (currentDb - targetDb);
        }
    }

    // Clamp
    currentDb = juce::jlimit (-60.f, 6.f, currentDb);

    // Peak hold
    if (currentDb >= peakDb)
    {
        peakDb = currentDb;
        holdCounter = 0.f;
    }
    else
    {
        holdCounter += static_cast<float> (elapsedMs);
        if (holdCounter > holdTimeMs)
        {
            // Decay peak
            float decayRate = 10.f; // dB per second
            peakDb -= decayRate * static_cast<float> (elapsedMs * 0.001);
            if (peakDb < currentDb)
                peakDb = currentDb;
        }
    }
}

juce::String MeterBallistics::getModeName (MeterMode m)
{
    switch (m)
    {
        case MeterMode::Peak:     return "PEAK";
        case MeterMode::RMS:      return "RMS";
        case MeterMode::EBU_R128: return "EBU";
        case MeterMode::VU:       return "VU";
        case MeterMode::PPM_BBC:  return "PPM";
        case MeterMode::PPM_EBU:  return "PPM";
    }
    return "VU";
}

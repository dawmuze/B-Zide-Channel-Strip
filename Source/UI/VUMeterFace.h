#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginStyle.h"
#include "MeterBallistics.h"

class VUMeterFace : public juce::Component
{
public:
    VUMeterFace();

    void paint (juce::Graphics& g) override;
    void resized() override {}

    void setLevel (float dB);
    void setMeterMode (MeterMode mode);

    MeterBallistics& getBallistics() { return ballistics; }
    float getCurrentDb() const { return ballistics.getCurrentDb(); }

private:
    void loadFaceImage();
    void drawNeedle (juce::Graphics& g, juce::Point<float> pivot, float radius, float angleDeg);

    MeterBallistics ballistics;
    double lastTimeMs = 0.0;
    juce::Image faceImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMeterFace)
};

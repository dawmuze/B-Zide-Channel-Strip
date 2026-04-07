#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class RotaryKnob : public juce::Component
{
public:
    RotaryKnob();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    float getValue() const { return value; }
    void setValue (float v);

    std::function<void (float)> onValueChange;

private:
    float value = 0.f;
    float minVal = -12.f;
    float maxVal = 12.f;
    int dragStartY = 0;
    float dragStartValue = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};

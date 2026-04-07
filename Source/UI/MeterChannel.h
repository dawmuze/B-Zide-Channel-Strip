#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "VUMeterFace.h"
#include "PluginStyle.h"

class MeterChannel : public juce::Component
{
public:
    MeterChannel();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setLevel (float dB);
    void setMeterMode (MeterMode mode);

    VUMeterFace& getMeterFace() { return meterFace; }

private:
    VUMeterFace meterFace;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterChannel)
};

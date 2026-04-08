#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "SpectrumAnalyzer.h"

class AnalyzerWindow : public juce::DocumentWindow
{
public:
    AnalyzerWindow(BZideProcessor& proc)
        : DocumentWindow("B-Zide EQ Analyzer",
                          juce::Colour(0xFF0A0A10),
                          DocumentWindow::closeButton)
    {
        setContentOwned(new SpectrumAnalyzer(proc), true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
        centreWithSize(620, 340);
        setVisible(true);
        setAlwaysOnTop(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerWindow)
};

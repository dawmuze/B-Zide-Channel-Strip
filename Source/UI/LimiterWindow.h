#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "LimiterPanel.h"

class LimiterWindow : public juce::DocumentWindow
{
public:
    LimiterWindow(BZideProcessor& proc)
        : DocumentWindow("B-Zide Limiter",
                          juce::Colour(0xFF0A0A10),
                          DocumentWindow::closeButton)
    {
        setContentOwned(new LimiterPanel(proc), true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
        centreWithSize(420, 220);
        setVisible(true);
        setAlwaysOnTop(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterWindow)
};

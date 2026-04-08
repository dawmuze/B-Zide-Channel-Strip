#pragma once
#include "ChannelSection.h"

class InsertSection : public ChannelSection
{
public:
    InsertSection()
        : ChannelSection(SectionId::INSERT, "INSERT", 140, false)
    {}

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // Rotated "No Insert" text
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        auto bounds = getLocalBounds();
        int totalH = bounds.getHeight();
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(
            -juce::MathConstants<float>::halfPi,
            (float)(sectionWidth / 2),
            (float)(totalH / 2)));
        g.drawText("No Insert", 0, totalH / 2 - 50, sectionWidth, 100, juce::Justification::centred);
        g.restoreState();
    }
};

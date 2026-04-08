#pragma once
#include "ChannelSection.h"

class DS2Section : public ChannelSection
{
public:
    DS2Section(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::DS2, "DE-ESSER", 140, true)
    {
        dsBypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "ds_bypass", bypassBtn);

        setupKnob(dsFreq1);   dsFreq1Att   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ds_freq1", dsFreq1);
        setupKnob(dsThresh1); dsThresh1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ds_thresh1", dsThresh1);
        setupKnob(dsFreq2);   dsFreq2Att   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ds_freq2", dsFreq2);
        setupKnob(dsThresh2); dsThresh2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ds_thresh2", dsThresh2);

    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // Divider between BAND 1 and BAND 2
        g.setColour(juce::Colour(0xFF333338));
        g.drawHorizontalLine(dsThresh1.getBottom() + 6, 8.0f, (float)(sectionWidth - 8));
    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        // BAND 1 header
        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("BAND 1", 0, dsFreq1.getY() - 28, sectionWidth, 12, juce::Justification::centred);
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(g, dsFreq1, "FREQ");
        labelAbove(g, dsThresh1, "THRESH");

        // BAND 2 header
        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("BAND 2", 0, dsFreq2.getY() - 28, sectionWidth, 12, juce::Justification::centred);
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(g, dsFreq2, "FREQ");
        labelAbove(g, dsThresh2, "THRESH");
    }

    void resizeSectionContent() override
    {
        int knobSize = 52;
        int labelGap = 38;
        int rowGap = 10;
        int y = getContentStartY();
        y += labelGap + 16;   // sub-header "BAND 1"
        centerKnob(dsFreq1, y, knobSize);   y += knobSize + labelGap + rowGap;
        centerKnob(dsThresh1, y, knobSize);  y += knobSize + labelGap + 24;
        centerKnob(dsFreq2, y, knobSize);    y += knobSize + labelGap + rowGap;
        centerKnob(dsThresh2, y, knobSize);
    }

private:
    juce::Slider dsFreq1, dsThresh1, dsFreq2, dsThresh2;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BA> dsBypassAtt;
    std::unique_ptr<SA> dsFreq1Att, dsThresh1Att, dsFreq2Att, dsThresh2Att;
};

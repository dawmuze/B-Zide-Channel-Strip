#pragma once
#include "ChannelSection.h"

class CompSection : public ChannelSection
{
public:
    CompSection(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::COMP, "COMPRESSOR", 140, true)
    {
        compBypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "comp_bypass", bypassBtn);

        compType.addItemList({"VCA", "FET", "OPT"}, 1);
        addAndMakeVisible(compType);
        compTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "comp_type", compType);

        setupKnob(compThresh);  compThreshAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "comp_threshold", compThresh);
        setupKnob(compRatio);   compRatioAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "comp_ratio", compRatio);
        setupKnob(compAttack);  compAttackAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "comp_attack", compAttack);
        setupKnob(compRelease); compReleaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "comp_release", compRelease);
        setupKnob(compMakeup);  compMakeupAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "comp_makeup", compMakeup);
        setupKnob(compMix);     compMixAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "comp_mix", compMix);

    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF333338));
            g.drawHorizontalLine(divY, 8.0f, (float)(sectionWidth - 8));
        };
        drawDiv(compThresh.getBottom() + 4);
        drawDiv(compRatio.getBottom() + 4);
        drawDiv(compAttack.getBottom() + 4);
    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(g, compThresh, "THRESH");
        labelAbove(g, compRatio, "RATIO");
        labelBetween(g, compAttack, compRelease, "ATK", "REL");
        labelBetween(g, compMakeup, compMix, "GAIN", "MIX");
    }

    void resizeSectionContent() override
    {
        int knobSize = 52;
        int knobSmall = 42;
        int labelGap = 38;
        int rowGap = 10;
        int y = getContentStartY();
        compType.setBounds(8, y, sectionWidth - 16, 22); y += 28;
        y += labelGap;
        centerKnob(compThresh, y, knobSize);                     y += knobSize + labelGap + rowGap;
        centerKnob(compRatio, y, knobSmall);                     y += knobSmall + labelGap + rowGap;
        centerKnobPair(compAttack, compRelease, y, knobSmall);   y += knobSmall + labelGap + rowGap;
        centerKnobPair(compMakeup, compMix, y, knobSmall);
    }

private:
    juce::ComboBox compType;
    juce::Slider compThresh, compRatio, compAttack, compRelease, compMakeup, compMix;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<BA> compBypassAtt;
    std::unique_ptr<CA> compTypeAtt;
    std::unique_ptr<SA> compThreshAtt, compRatioAtt, compAttackAtt, compReleaseAtt, compMakeupAtt, compMixAtt;
};

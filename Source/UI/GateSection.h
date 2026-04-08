#pragma once
#include "ChannelSection.h"

class GateSection : public ChannelSection
{
public:
    GateSection(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::GATE, "GATE", 140, true)
    {
        gateBypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "gate_bypass", bypassBtn);

        gateType.addItemList({"GATE", "EXP"}, 1);
        addAndMakeVisible(gateType);
        gateTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "gate_type", gateType);

        setupKnob(gateThresh);  gateThreshAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gate_threshold", gateThresh);
        setupKnob(gateAtten);   gateAttenAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gate_atten", gateAtten);
        setupKnob(gateFloor);   gateFloorAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gate_floor", gateFloor);
        setupKnob(gateAttack);  gateAttackAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gate_attack", gateAttack);
        setupKnob(gateRelease); gateReleaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gate_release", gateRelease);

    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF333338));
            g.drawHorizontalLine(divY, 8.0f, (float)(sectionWidth - 8));
        };
        drawDiv(gateThresh.getBottom() + 4);
        drawDiv(gateAtten.getBottom() + 4);
        drawDiv(gateFloor.getBottom() + 4);
    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(g, gateThresh, "THRESH");
        labelAbove(g, gateAtten, "ATTEN");
        labelAbove(g, gateFloor, "FLOOR");
        labelBetween(g, gateAttack, gateRelease, "ATK", "REL");
    }

    void resizeSectionContent() override
    {
        int knobSize = 52;
        int knobSmall = 42;
        int labelGap = 38;
        int rowGap = 10;
        int y = getContentStartY();
        gateType.setBounds(8, y, sectionWidth - 16, 22); y += 30;
        y += labelGap;
        centerKnob(gateThresh, y, knobSize);                     y += knobSize + labelGap + rowGap;
        centerKnob(gateAtten, y, knobSmall);                     y += knobSmall + labelGap + rowGap;
        centerKnob(gateFloor, y, knobSmall);                     y += knobSmall + labelGap + rowGap;
        centerKnobPair(gateAttack, gateRelease, y, knobSmall);
    }

private:
    juce::ComboBox gateType;
    juce::Slider gateThresh, gateAtten, gateFloor, gateAttack, gateRelease;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<BA> gateBypassAtt;
    std::unique_ptr<CA> gateTypeAtt;
    std::unique_ptr<SA> gateThreshAtt, gateAttenAtt, gateFloorAtt, gateAttackAtt, gateReleaseAtt;
};

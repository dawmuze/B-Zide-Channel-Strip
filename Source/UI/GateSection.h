#pragma once
#include "ChannelSection.h"

class GateSection : public ChannelSection
{
public:
    GateSection(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::GATE, "GATE", 140, true),
          apvtsRef(apvts)
    {
        gateBypassAtt = std::make_unique<BA>(apvts, "gate_bypass", bypassBtn);

        // Type buttons — GATE / EXP (radio group)
        for (auto* btn : { &gateTypeBtn, &expTypeBtn })
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(2002);
            addAndMakeVisible(btn);
        }
        gateTypeBtn.onClick = [this]() { if (gateTypeBtn.getToggleState()) setTypeParam(0); };
        expTypeBtn.onClick  = [this]() { if (expTypeBtn.getToggleState())  setTypeParam(1); };

        int initType = (int)*apvts.getRawParameterValue("gate_type");
        if (initType == 0) gateTypeBtn.setToggleState(true, juce::dontSendNotification);
        else               expTypeBtn.setToggleState(true, juce::dontSendNotification);

        // Knobs — WHITE (Threshold, Depth)
        juce::Colour whiteKnob(0xFFE8E8EC);
        setupKnob(gateThresh, " dB");
        gateThresh.setColour(juce::Slider::rotarySliderFillColourId, whiteKnob);
        gateThreshAtt = std::make_unique<SA>(apvts, "gate_threshold", gateThresh);

        setupKnob(gateAtten, " dB");
        gateAtten.setColour(juce::Slider::rotarySliderFillColourId, whiteKnob);
        gateAttenAtt = std::make_unique<SA>(apvts, "gate_atten", gateAtten);

        // Knobs — hidden but APVTS attachments kept alive (Floor, Attack, Release)
        setupKnob(gateFloor, " dB");
        gateFloorAtt = std::make_unique<SA>(apvts, "gate_floor", gateFloor);
        gateFloor.setVisible(false);

        setupKnob(gateAttack, " ms");
        gateAttackAtt = std::make_unique<SA>(apvts, "gate_attack", gateAttack);
        gateAttack.setVisible(false);

        setupKnob(gateRelease, " ms");
        gateReleaseAtt = std::make_unique<SA>(apvts, "gate_release", gateRelease);
        gateRelease.setVisible(false);

        // FAST / PEAK toggle buttons — wired to gate_fast and gate_peak
        fastBtn.setClickingTogglesState(true);
        addAndMakeVisible(fastBtn);
        fastBtn.setToggleState(*apvts.getRawParameterValue("gate_fast") > 0.5f, juce::dontSendNotification);
        fastBtn.onClick = [this]() { apvtsRef.getParameter("gate_fast")->setValueNotifyingHost(fastBtn.getToggleState() ? 1.0f : 0.0f); };

        peakBtn.setClickingTogglesState(true);
        addAndMakeVisible(peakBtn);
        peakBtn.setToggleState(*apvts.getRawParameterValue("gate_peak") > 0.5f, juce::dontSendNotification);
        peakBtn.onClick = [this]() { apvtsRef.getParameter("gate_peak")->setValueNotifyingHost(peakBtn.getToggleState() ? 1.0f : 0.0f); };

        // SC button — wired to gate_sc
        scBtn.setClickingTogglesState(true);
        addAndMakeVisible(scBtn);
        scBtn.setToggleState(*apvts.getRawParameterValue("gate_sc") > 0.5f, juce::dontSendNotification);
        scBtn.onClick = [this]() { apvtsRef.getParameter("gate_sc")->setValueNotifyingHost(scBtn.getToggleState() ? 1.0f : 0.0f); };
    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // Dark blue/slate faceplate background (Neve 545-inspired)
        auto area = getContentArea();
        g.setColour(juce::Colour(0xFF1A2030));
        g.fillRect(area);

        // Inner bevel with blue tint
        g.setColour(juce::Colour(0x18000000));
        g.drawRect(area, 1);
        g.setColour(juce::Colour(0x18AABBCC));
        g.drawHorizontalLine(area.getY() + 1, (float)area.getX() + 1, (float)area.getRight() - 1);

        float dL = (float)(area.getX() + 4);
        float dR = (float)(area.getRight() - 4);

        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF0E1220));
            g.drawHorizontalLine(divY, dL, dR);
            g.setColour(juce::Colour(0x18AABBCC));
            g.drawHorizontalLine(divY + 1, dL, dR);
        };

        // Vertical divider helper (3D groove, blue-tinted)
        int vDivX = sectionWidth - 38;

        auto drawVDiv = [&](int vx, int vTop, int vBot) {
            g.setColour(juce::Colour(0xFF141828));
            g.drawVerticalLine(vx - 1, (float)vTop, (float)vBot);
            g.setColour(juce::Colour(0xFF0E1220));
            g.drawVerticalLine(vx, (float)vTop, (float)vBot);
            g.setColour(juce::Colour(0xFF2A3040));
            g.drawVerticalLine(vx + 1, (float)vTop, (float)vBot);
        };

        // THRESHOLD header + divider
        if (threshHeaderY > 0)
        {
            drawDiv(threshHeaderY - 4);
            drawHeader(g, threshHeaderY, "THRESHOLD");
        }

        // Divider below threshold
        if (gateThresh.getHeight() > 0)
            drawDiv(gateThresh.getBottom() + 4);

        // DEPTH header
        if (depthHeaderY > 0)
            drawHeader(g, depthHeaderY, "DEPTH");

        // Divider below depth
        if (gateAtten.getHeight() > 0)
            drawDiv(gateAtten.getBottom() + 4);

        // GR METER header + vertical meter strip (centered, below knobs)
        if (grMeterHeaderY > 0)
            drawHeader(g, grMeterHeaderY, "REDUCTION");

        if (!grMeterBounds.isEmpty())
        {
            auto mb = grMeterBounds;
            g.setColour(juce::Colour(0xFF333340));
            g.setFont(juce::Font(juce::FontOptions(6.5f)).boldened());
            g.drawText("GR", mb.getX() - 2, mb.getY() - 10, mb.getWidth() + 4, 9, juce::Justification::centred);

            int numLeds = 8;
            float ledH = (float)mb.getHeight() / (float)numLeds;
            for (int i = 0; i < numLeds; ++i)
            {
                float ly = (float)mb.getY() + (float)i * ledH;
                juce::Colour c = (i < 3) ? juce::Colour(0xFF1A3A1A) : (i < 5) ? juce::Colour(0xFF3A3A1A) : juce::Colour(0xFF3A1A1A);
                g.setColour(c);
                g.fillRoundedRectangle((float)mb.getX(), ly + 1.0f, (float)mb.getWidth(), ledH - 2.0f, 1.5f);
            }
        }

        // Divider below meter
        if (!grMeterBounds.isEmpty())
            drawDiv(grMeterBounds.getBottom() + 4);

        // CONTROL header
        if (controlHeaderY > 0)
        {
            drawDiv(controlHeaderY - 4);
            drawHeader(g, controlHeaderY, "CONTROL");
        }

        // Divider below control
        if (controlHeaderY > 0)
            drawDiv(controlHeaderY + 22 + 28 + 4);

        // SIDECHAIN header
        if (scHeaderY > 0)
        {
            drawDiv(scHeaderY - 4);
            drawHeader(g, scHeaderY, "SIDECHAIN");
        }
    }

    void paintSectionLabels(juce::Graphics&) override {}

    void resizeSectionContent() override
    {
        int y = getContentStartY();

        // Row 1: GATE / EXP buttons — horizontal, centered
        int typeBtnW = (sectionWidth - 24) / 2;
        int typeBtnH = 26;
        int typeStartX = (sectionWidth - (typeBtnW * 2 + 4)) / 2;
        gateTypeBtn.setBounds(typeStartX, y, typeBtnW, typeBtnH);
        expTypeBtn.setBounds(typeStartX + typeBtnW + 4, y, typeBtnW, typeBtnH);
        y += typeBtnH + 8;

        // Row 2: THRESHOLD — big knob centered in full width
        int bigKnob = 100;
        threshHeaderY = y; y += 22;
        centerKnob(gateThresh, y, bigKnob);
        y += bigKnob + 22;

        // Row 3: DEPTH — big knob centered
        depthHeaderY = y; y += 22;
        centerKnob(gateAtten, y, bigKnob);
        y += bigKnob + 22;

        // Row 4: GR METER — centered vertical strip below knobs
        grMeterHeaderY = y; y += 22;
        int grW = 18;
        int grH = 80;
        grMeterBounds = juce::Rectangle<int>((sectionWidth - grW) / 2, y, grW, grH);
        y += grH + 10;

        // Row 5: CONTROL — FAST / PEAK buttons centered
        controlHeaderY = y; y += 22;
        int ctrlBtnW = 50;
        int ctrlGap = 6;
        int ctrlTotalW = ctrlBtnW * 2 + ctrlGap;
        int ctrlStartX = (sectionWidth - ctrlTotalW) / 2;
        fastBtn.setBounds(ctrlStartX, y, ctrlBtnW, 26);
        peakBtn.setBounds(ctrlStartX + ctrlBtnW + ctrlGap, y, ctrlBtnW, 26);
        y += 32;

        // Row 5: SC button
        scHeaderY = y; y += 22;
        int scW = sectionWidth - 24;
        scBtn.setBounds((sectionWidth - scW) / 2, y, scW, 22);

        // Hide unused knobs (APVTS attachments remain alive)
        gateFloor.setVisible(false);
        gateAttack.setVisible(false);
        gateRelease.setVisible(false);
    }

    // Blue-tinted header helper (dark background, light text)
    void drawHeader(juce::Graphics& g, int headerY, const juce::String& text)
    {
        int hdrH = 18;
        auto hdrArea = juce::Rectangle<int>(6, headerY, sectionWidth - 12, hdrH);
        g.setColour(juce::Colour(0xFF141828));
        g.fillRoundedRectangle(hdrArea.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xFF1A1E30));
        g.fillRoundedRectangle(hdrArea.reduced(1).toFloat(), 2.0f);
        g.setColour(juce::Colour(0xFFCCCCDD));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        g.drawText(text, hdrArea, juce::Justification::centred);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // Knobs
    juce::Slider gateThresh, gateAtten, gateFloor, gateAttack, gateRelease;

    // Type buttons
    juce::TextButton gateTypeBtn { "GATE" };
    juce::TextButton expTypeBtn  { "EXP" };

    // Control buttons
    juce::TextButton fastBtn { "FAST" };
    juce::TextButton peakBtn { "PEAK" };

    // Sidechain button
    juce::TextButton scBtn { "SC" };

    // Saved Y positions for painted elements
    int threshHeaderY = 0;
    int depthHeaderY = 0;
    int grMeterHeaderY = 0;
    int controlHeaderY = 0;
    int scHeaderY = 0;
    juce::Rectangle<int> grMeterBounds;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BA> gateBypassAtt;
    std::unique_ptr<SA> gateThreshAtt, gateAttenAtt, gateFloorAtt, gateAttackAtt, gateReleaseAtt;

    void setTypeParam(int value)
    {
        if (auto* param = apvtsRef.getParameter("gate_type"))
            param->setValueNotifyingHost(param->convertTo0to1((float)value));
    }
};

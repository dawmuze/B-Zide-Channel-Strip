#pragma once
#include "ChannelSection.h"

class CompSection : public ChannelSection
{
public:
    CompSection(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::COMP, "COMPRESSOR", 140, true),
          apvtsRef(apvts)
    {
        compBypassAtt = std::make_unique<BA>(apvts, "comp_bypass", bypassBtn);

        // Type buttons - VCA / FET / OPT (radio group)
        for (auto* btn : { &vcaBtn, &fetBtn, &optBtn })
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(2001);
            addAndMakeVisible(btn);
        }
        vcaBtn.onClick = [this]() { if (vcaBtn.getToggleState()) setTypeParam(0); };
        fetBtn.onClick = [this]() { if (fetBtn.getToggleState()) setTypeParam(1); };
        optBtn.onClick = [this]() { if (optBtn.getToggleState()) setTypeParam(2); };

        int initType = (int)*apvts.getRawParameterValue("comp_type");
        if (initType == 0)      vcaBtn.setToggleState(true, juce::dontSendNotification);
        else if (initType == 1) fetBtn.setToggleState(true, juce::dontSendNotification);
        else                    optBtn.setToggleState(true, juce::dontSendNotification);

        // Knobs - WHITE (Threshold, Ratio, Attack, Release)
        juce::Colour whiteKnob(0xFFFFFFFF);
        setupKnob(compThresh, " dB");
        compThresh.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF111118));
        compThreshAtt = std::make_unique<SA>(apvts, "comp_threshold", compThresh);

        setupKnob(compRatio);
        compRatio.setColour(juce::Slider::rotarySliderFillColourId, whiteKnob);
        compRatioAtt = std::make_unique<SA>(apvts, "comp_ratio", compRatio);

        setupKnob(compAttack, " ms");
        compAttack.setColour(juce::Slider::rotarySliderFillColourId, whiteKnob);
        compAttackAtt = std::make_unique<SA>(apvts, "comp_attack", compAttack);

        setupKnob(compRelease, " ms");
        compRelease.setColour(juce::Slider::rotarySliderFillColourId, whiteKnob);
        compReleaseAtt = std::make_unique<SA>(apvts, "comp_release", compRelease);

        // Knob - RED (Makeup Gain)
        setupKnob(compMakeup, " dB");
        compMakeup.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFCC2222));
        compMakeupAtt = std::make_unique<SA>(apvts, "comp_makeup", compMakeup);

        // MIX - small knob
        setupKnob(compMix, "%");
        compMixAtt = std::make_unique<SA>(apvts, "comp_mix", compMix);

        // SC and HPF — both toggle comp_sc_hpf (SC = sidechain HPF enable)
        scBtn.setClickingTogglesState(true);
        addAndMakeVisible(scBtn);
        scBtn.setToggleState(*apvts.getRawParameterValue("comp_sc_hpf") > 0.5f, juce::dontSendNotification);
        scBtn.onClick = [this]() {
            apvtsRef.getParameter("comp_sc_hpf")->setValueNotifyingHost(scBtn.getToggleState() ? 1.0f : 0.0f);
            hpfBtn.setToggleState(scBtn.getToggleState(), juce::dontSendNotification);
        };
        hpfBtn.setClickingTogglesState(true);
        addAndMakeVisible(hpfBtn);
        hpfBtn.setToggleState(*apvts.getRawParameterValue("comp_sc_hpf") > 0.5f, juce::dontSendNotification);
        hpfBtn.onClick = [this]() {
            apvtsRef.getParameter("comp_sc_hpf")->setValueNotifyingHost(hpfBtn.getToggleState() ? 1.0f : 0.0f);
            scBtn.setToggleState(hpfBtn.getToggleState(), juce::dontSendNotification);
        };

        // F/F and F/B — radio group, wired to comp_topology
        for (auto* b : { &ffBtn, &fbBtn })
        {
            b->setClickingTogglesState(true);
            b->setRadioGroupId(3001);
            addAndMakeVisible(b);
        }
        {
            int initTopo = (int)*apvts.getRawParameterValue("comp_topology");
            if (initTopo == 0) ffBtn.setToggleState(true, juce::dontSendNotification);
            else fbBtn.setToggleState(true, juce::dontSendNotification);
        }
        ffBtn.onClick = [this]() { if (ffBtn.getToggleState()) apvtsRef.getParameter("comp_topology")->setValueNotifyingHost(0.0f); };
        fbBtn.onClick = [this]() { if (fbBtn.getToggleState()) apvtsRef.getParameter("comp_topology")->setValueNotifyingHost(1.0f); };

        // RMS and PEAK — radio group, wired to comp_detect
        for (auto* b : { &rmsBtn, &peakBtn })
        {
            b->setClickingTogglesState(true);
            b->setRadioGroupId(3002);
            addAndMakeVisible(b);
        }
        {
            int initDetect = (int)*apvts.getRawParameterValue("comp_detect");
            if (initDetect == 0) rmsBtn.setToggleState(true, juce::dontSendNotification);
            else peakBtn.setToggleState(true, juce::dontSendNotification);
        }
        rmsBtn.onClick  = [this]() { if (rmsBtn.getToggleState())  apvtsRef.getParameter("comp_detect")->setValueNotifyingHost(0.0f); };
        peakBtn.onClick = [this]() { if (peakBtn.getToggleState()) apvtsRef.getParameter("comp_detect")->setValueNotifyingHost(1.0f); };
    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // Light faceplate background (Neve-style)
        auto area = getContentArea();
        g.setColour(juce::Colour(0xFFD0D0D4));
        g.fillRect(area);

        // Subtle inner bevel on faceplate
        g.setColour(juce::Colour(0x18000000));
        g.drawRect(area, 1);
        g.setColour(juce::Colour(0x20FFFFFF));
        g.drawHorizontalLine(area.getY() + 1, (float)area.getX() + 1, (float)area.getRight() - 1);

        float dL = (float)(area.getX() + 4);
        float dR = (float)(area.getRight() - 4);

        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFFB0B0B4));
            g.drawHorizontalLine(divY, dL, dR);
            g.setColour(juce::Colour(0x18FFFFFF));
            g.drawHorizontalLine(divY + 1, dL, dR);
        };

        // Headers with dividers for each knob
        if (threshHeaderY > 0) { drawDiv(threshHeaderY - 4); drawHeader(g, threshHeaderY, "THRESHOLD"); }
        if (ratioHeaderY > 0)  { drawDiv(ratioHeaderY - 4);  drawHeader(g, ratioHeaderY, "RATIO"); }

        // LEVEL meter strip (next to RATIO)
        if (!levelMeterBounds.isEmpty())
        {
            auto mb = levelMeterBounds;
            g.setColour(juce::Colour(0xFF333340));
            g.setFont(juce::Font(juce::FontOptions(6.5f)).boldened());
            g.drawText("LVL", mb.getX() - 2, mb.getY() - 10, mb.getWidth() + 4, 9, juce::Justification::centred);

            int numLeds = 8;
            float ledH = (float)mb.getHeight() / (float)numLeds;
            for (int i = 0; i < numLeds; ++i)
            {
                float ly = (float)mb.getY() + (float)i * ledH;
                // All off/dim for now (green tones)
                juce::Colour c = (i < 2) ? juce::Colour(0xFF1A3A1A) : (i < 5) ? juce::Colour(0xFF1A2A1A) : juce::Colour(0xFF1A3A1A);
                g.setColour(c);
                g.fillRoundedRectangle((float)mb.getX(), ly + 1.0f, (float)mb.getWidth(), ledH - 2.0f, 1.5f);
            }
        }

        // GR meter strip (next to ATTACK)
        if (!grMeterBounds.isEmpty())
        {
            auto mb = grMeterBounds;
            g.setColour(juce::Colour(0xFF333340));
            g.setFont(juce::Font(juce::FontOptions(6.5f)).boldened());
            g.drawText("GR", mb.getX() - 2, mb.getY() - 10, mb.getWidth() + 4, 9, juce::Justification::centred);

            int numLeds = 8;
            float ledH = (float)mb.getHeight() / (float)numLeds;
            const int grValues[] = { 1, 2, 4, 6, 8, 10, 14, 22 };
            for (int i = 0; i < numLeds; ++i)
            {
                float ly = (float)mb.getY() + (float)i * ledH;
                juce::Colour c = (i < 3) ? juce::Colour(0xFF1A3A1A) : (i < 5) ? juce::Colour(0xFF3A3A1A) : juce::Colour(0xFF3A1A1A);
                g.setColour(c);
                g.fillRoundedRectangle((float)mb.getX(), ly + 1.0f, (float)mb.getWidth(), ledH - 2.0f, 1.5f);
            }
        }

        if (attackHeaderY > 0)  { drawDiv(attackHeaderY - 4);  drawHeader(g, attackHeaderY, "ATTACK"); }
        if (releaseHeaderY > 0) { drawDiv(releaseHeaderY - 4); drawHeader(g, releaseHeaderY, "RELEASE"); }
        if (makeupHeaderY > 0)  { drawDiv(makeupHeaderY - 4);  drawHeader(g, makeupHeaderY, "MAKE-UP GAIN"); }

        // Divider below makeup
        if (compMakeup.getHeight() > 0)
            drawDiv(compMakeup.getBottom() + 8);

        // Vertical dividers with 3D depth (groove effect)
        auto drawVDiv = [&](int vx, int vTop, int vBot) {
            // Dark shadow line (left side of groove)
            g.setColour(juce::Colour(0xFF888890));
            g.drawVerticalLine(vx - 1, (float)vTop, (float)vBot);
            // Dark center
            g.setColour(juce::Colour(0xFF707078));
            g.drawVerticalLine(vx, (float)vTop, (float)vBot);
            // Highlight (right side of groove — light catches the edge)
            g.setColour(juce::Colour(0xFFE0E0E4));
            g.drawVerticalLine(vx + 1, (float)vTop, (float)vBot);
        };

        int vDivX = sectionWidth - 38;

        // Vertical divider next to THRESHOLD (between knob and SC/HPF)
        if (scBtn.getHeight() > 0)
            drawVDiv(vDivX, scBtn.getY() - 2, hpfBtn.getBottom() + 2);

        // Vertical divider next to RATIO (between knob and LVL meter)
        if (!levelMeterBounds.isEmpty())
            drawVDiv(vDivX, levelMeterBounds.getY(), levelMeterBounds.getBottom() + 2);

        // Vertical divider next to ATTACK (between knob and GR meter)
        if (!grMeterBounds.isEmpty())
            drawVDiv(vDivX, grMeterBounds.getY(), grMeterBounds.getBottom() + 2);

        // Vertical divider next to RELEASE (between knob and F/F, F/B)
        if (ffBtn.getHeight() > 0)
            drawVDiv(vDivX, ffBtn.getY() - 2, fbBtn.getBottom() + 2);

        // Vertical divider next to MAKE-UP GAIN (between knob and RMS/PEAK)
        if (rmsBtn.getHeight() > 0)
            drawVDiv(vDivX, rmsBtn.getY() - 2, peakBtn.getBottom() + 2);
    }

    void paintSectionLabels(juce::Graphics&) override {}

    void resizeSectionContent() override
    {
        int y = getContentStartY();
        int cx = 3; // content left edge (shadow gap)

        // Row 1: VCA / FET / OPT buttons
        int typeBtnW = (sectionWidth - 20) / 3;
        int typeBtnH = 30;
        int typeGap = 2;
        int typeTotalW = typeBtnW * 3 + typeGap * 2;
        int typeStartX = (sectionWidth - typeTotalW) / 2;
        vcaBtn.setBounds(typeStartX, y, typeBtnW, typeBtnH);
        fetBtn.setBounds(typeStartX + typeBtnW + typeGap, y, typeBtnW, typeBtnH);
        optBtn.setBounds(typeStartX + (typeBtnW + typeGap) * 2, y, typeBtnW, typeBtnH);
        y += typeBtnH + 8;

        // Side buttons/meters position
        int vDivX = sectionWidth - 38; // vertical divider X
        int sbX = vDivX + 5;           // side buttons start after divider
        int sbW = 28;
        int sbH = 18;

        // Knob area = from left edge (3) to divider (vDivX-2)
        int knobAreaLeft = 3;
        int knobAreaW = vDivX - knobAreaLeft - 2;

        int bigKnob = 100;
        int knobPad = 14;
        int knobW = bigKnob + knobPad;
        int knobH = bigKnob + 22;
        // Center knob in the knob area
        int knobX = knobAreaLeft + (knobAreaW - knobW) / 2;

        // THRESHOLD — centered in knob area + SC/HPF right
        threshHeaderY = y; y += 22;
        compThresh.setBounds(knobX, y, knobW, knobH);
        scBtn.setBounds(sbX, y + bigKnob / 2 - sbH - 2, sbW, sbH);
        hpfBtn.setBounds(sbX, y + bigKnob / 2 + 2, sbW, sbH);
        y += bigKnob + 22;

        // RATIO — centered in knob area + LEVEL meter centered in side area
        ratioHeaderY = y; y += 22;
        compRatio.setBounds(knobX, y, knobW, knobH);
        int sideAreaLeft = vDivX + 3;
        int sideAreaW = sectionWidth - 6 - sideAreaLeft; // right edge minus shadow gap
        int meterW = 18;
        int meterX = sideAreaLeft + (sideAreaW - meterW) / 2;
        levelMeterBounds = juce::Rectangle<int>(meterX, y + 6, meterW, bigKnob + 8);
        y += bigKnob + 22;

        // ATTACK — centered in knob area + GR meter centered in side area
        attackHeaderY = y; y += 22;
        compAttack.setBounds(knobX, y, knobW, knobH);
        grMeterBounds = juce::Rectangle<int>(meterX, y + 6, meterW, bigKnob + 8);
        grMeterY = y + 10;
        y += bigKnob + 22;

        // RELEASE — centered in knob area + F/F, F/B right
        releaseHeaderY = y; y += 22;
        compRelease.setBounds(knobX, y, knobW, knobH);
        ffBtn.setBounds(sbX, y + bigKnob / 2 - sbH - 2, sbW, sbH);
        fbBtn.setBounds(sbX, y + bigKnob / 2 + 2, sbW, sbH);
        y += bigKnob + 22;

        // MAKE-UP GAIN — centered in knob area + RMS/PEAK right
        int mkKnob = 50;
        int mkW = mkKnob + knobPad;
        int mkH = mkKnob + 22;
        int mkX = knobAreaLeft + (knobAreaW - mkW) / 2;
        makeupHeaderY = y; y += 22;
        compMakeup.setBounds(mkX, y, mkW, mkH);
        rmsBtn.setBounds(sbX, y + mkKnob / 2 - sbH - 2, sbW, sbH);
        peakBtn.setBounds(sbX, y + mkKnob / 2 + 2, sbW, sbH);
        y += mkKnob + 14;

        // Hide MIX knob (not needed)
        compMix.setVisible(false);
    }

    // Header helper for light background (dark panel)
    void drawHeader(juce::Graphics& g, int headerY, const juce::String& text)
    {
        int hdrH = 18;
        auto hdrArea = juce::Rectangle<int>(6, headerY, sectionWidth - 12, hdrH);
        g.setColour(juce::Colour(0xFFB0B0B4));
        g.fillRoundedRectangle(hdrArea.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xFFC0C0C4));
        g.fillRoundedRectangle(hdrArea.reduced(1).toFloat(), 2.0f);
        g.setColour(juce::Colour(0xFF333340));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        g.drawText(text, hdrArea, juce::Justification::centred);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // Knobs
    juce::Slider compThresh, compRatio, compAttack, compRelease, compMakeup, compMix;

    // Type buttons
    juce::TextButton vcaBtn { "VCA" };
    juce::TextButton fetBtn { "FET" };
    juce::TextButton optBtn { "OPT" };

    // SC and HPF buttons
    juce::TextButton scBtn  { "SC" };
    juce::TextButton hpfBtn { "HPF" };

    // Feed-Forward / Feed-Back buttons
    juce::TextButton ffBtn  { "F/F" };
    juce::TextButton fbBtn  { "F/B" };

    // RMS / Peak buttons
    juce::TextButton rmsBtn  { "RMS" };
    juce::TextButton peakBtn { "PEAK" };

    // Saved Y positions for dividers and GR meter
    int threshHeaderY = 0;
    int ratioHeaderY = 0;
    int attackHeaderY = 0;
    int releaseHeaderY = 0;
    int makeupHeaderY = 0;
    int grMeterY = 0;
    juce::Rectangle<int> grMeterBounds;
    juce::Rectangle<int> levelMeterBounds;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BA> compBypassAtt;
    std::unique_ptr<SA> compThreshAtt, compRatioAtt, compAttackAtt, compReleaseAtt, compMakeupAtt, compMixAtt;

    void setTypeParam(int value)
    {
        if (auto* param = apvtsRef.getParameter("comp_type"))
            param->setValueNotifyingHost(param->convertTo0to1((float)value));
    }
};

#pragma once
#include "ChannelSection.h"
#include "AnalyzerWindow.h"

class EQSection : public ChannelSection
{
public:
    EQSection(juce::AudioProcessorValueTreeState& apvts, BZideProcessor* proc = nullptr)
        : ChannelSection(SectionId::EQ, "EQUALIZER", 140, true),
          apvtsRef(apvts),
          processorRef(proc)
    {
        eqBypassAtt = std::make_unique<BA>(apvts, "eq_bypass", bypassBtn);

        // HIGH band — red tones (slightly different from default 0xFFDD2200)
        juce::Colour highCol(0xFFE83010);
        setupKnob(eqHighGain); eqHighGainAtt = std::make_unique<SA>(apvts, "eq_high_gain", eqHighGain);
        eqHighGain.setColour(juce::Slider::rotarySliderFillColourId, highCol);
        setupKnob(eqHighFreq); eqHighFreqAtt = std::make_unique<SA>(apvts, "eq_high_freq", eqHighFreq);
        eqHighFreq.setColour(juce::Slider::rotarySliderFillColourId, highCol.darker(0.3f));
        setupKnob(eqHighQ);
        eqHighQ.setColour(juce::Slider::rotarySliderFillColourId, highCol.darker(0.5f));
        eqHighQAtt = std::make_unique<SA>(apvts, "eq_high_q", eqHighQ);

        // MID band — green tones
        juce::Colour midCol(0xFF22c55e);
        setupKnob(eqMidGain);  eqMidGainAtt  = std::make_unique<SA>(apvts, "eq_mid_gain",  eqMidGain);
        eqMidGain.setColour(juce::Slider::rotarySliderFillColourId, midCol);
        setupKnob(eqMidFreq);  eqMidFreqAtt  = std::make_unique<SA>(apvts, "eq_mid_freq",  eqMidFreq);
        eqMidFreq.setColour(juce::Slider::rotarySliderFillColourId, midCol.darker(0.3f));
        setupKnob(eqMidQ);     eqMidQAtt     = std::make_unique<SA>(apvts, "eq_mid_q",     eqMidQ);
        eqMidQ.setColour(juce::Slider::rotarySliderFillColourId, midCol.darker(0.5f));

        // LOW band — blue tones
        juce::Colour lowCol(0xFF3b82f6);
        setupKnob(eqLowGain);  eqLowGainAtt  = std::make_unique<SA>(apvts, "eq_low_gain",  eqLowGain);
        eqLowGain.setColour(juce::Slider::rotarySliderFillColourId, lowCol);
        setupKnob(eqLowFreq);  eqLowFreqAtt  = std::make_unique<SA>(apvts, "eq_low_freq",  eqLowFreq);
        eqLowFreq.setColour(juce::Slider::rotarySliderFillColourId, lowCol.darker(0.3f));
        setupKnob(eqLowQ);
        eqLowQ.setColour(juce::Slider::rotarySliderFillColourId, lowCol.darker(0.5f));
        eqLowQAtt = std::make_unique<SA>(apvts, "eq_low_q", eqLowQ);

        // FILTERS — brown/amber tones
        juce::Colour filterCol(0xFF92400e);
        setupKnob(eqHpf);     eqHpfAtt      = std::make_unique<SA>(apvts, "eq_hpf",       eqHpf);
        eqHpf.setColour(juce::Slider::rotarySliderFillColourId, filterCol);
        setupKnob(eqLpf);     eqLpfAtt      = std::make_unique<SA>(apvts, "eq_lpf",       eqLpf);
        eqLpf.setColour(juce::Slider::rotarySliderFillColourId, filterCol.darker(0.2f));

        // Curve type buttons — HIGH (radio group 2001)
        for (auto* b : { &highCurve1, &highCurve2, &highCurve3 }) {
            b->setClickingTogglesState(true);
            b->setRadioGroupId(2001);
            addAndMakeVisible(b);
        }
        // Init from param: default High Shelf = 2 → highCurve3
        {
            int initHigh = (int)*apvts.getRawParameterValue("eq_high_type");
            if (initHigh == 0) highCurve1.setToggleState(true, juce::dontSendNotification);
            else if (initHigh == 1) highCurve2.setToggleState(true, juce::dontSendNotification);
            else highCurve3.setToggleState(true, juce::dontSendNotification);
        }
        highCurve1.onClick = [this]() { if (highCurve1.getToggleState()) apvtsRef.getParameter("eq_high_type")->setValueNotifyingHost(0.0f); };
        highCurve2.onClick = [this]() { if (highCurve2.getToggleState()) apvtsRef.getParameter("eq_high_type")->setValueNotifyingHost(0.5f); };
        highCurve3.onClick = [this]() { if (highCurve3.getToggleState()) apvtsRef.getParameter("eq_high_type")->setValueNotifyingHost(1.0f); };

        // Curve type buttons — MID (radio group 2002)
        for (auto* b : { &midCurve1, &midCurve2, &midCurve3 }) {
            b->setClickingTogglesState(true);
            b->setRadioGroupId(2002);
            addAndMakeVisible(b);
        }
        {
            int initMid = (int)*apvts.getRawParameterValue("eq_mid_type");
            if (initMid == 0) midCurve1.setToggleState(true, juce::dontSendNotification);
            else if (initMid == 1) midCurve2.setToggleState(true, juce::dontSendNotification);
            else midCurve3.setToggleState(true, juce::dontSendNotification);
        }
        midCurve1.onClick = [this]() { if (midCurve1.getToggleState()) apvtsRef.getParameter("eq_mid_type")->setValueNotifyingHost(0.0f); };
        midCurve2.onClick = [this]() { if (midCurve2.getToggleState()) apvtsRef.getParameter("eq_mid_type")->setValueNotifyingHost(0.5f); };
        midCurve3.onClick = [this]() { if (midCurve3.getToggleState()) apvtsRef.getParameter("eq_mid_type")->setValueNotifyingHost(1.0f); };

        // Curve type buttons — LOW (radio group 2003)
        for (auto* b : { &lowCurve1, &lowCurve2, &lowCurve3 }) {
            b->setClickingTogglesState(true);
            b->setRadioGroupId(2003);
            addAndMakeVisible(b);
        }
        {
            int initLow = (int)*apvts.getRawParameterValue("eq_low_type");
            if (initLow == 0) lowCurve1.setToggleState(true, juce::dontSendNotification);
            else if (initLow == 1) lowCurve2.setToggleState(true, juce::dontSendNotification);
            else lowCurve3.setToggleState(true, juce::dontSendNotification);
        }
        lowCurve1.onClick = [this]() { if (lowCurve1.getToggleState()) apvtsRef.getParameter("eq_low_type")->setValueNotifyingHost(0.0f); };
        lowCurve2.onClick = [this]() { if (lowCurve2.getToggleState()) apvtsRef.getParameter("eq_low_type")->setValueNotifyingHost(0.5f); };
        lowCurve3.onClick = [this]() { if (lowCurve3.getToggleState()) apvtsRef.getParameter("eq_low_type")->setValueNotifyingHost(1.0f); };

        // Analyzer button
        analyzerBtn.setButtonText("ANALYZER");
        analyzerBtn.onClick = [this]() {
            if (processorRef != nullptr)
            {
                if (analyzerWindow == nullptr || !analyzerWindow->isVisible())
                    analyzerWindow = std::make_unique<AnalyzerWindow>(*processorRef);
                else
                    analyzerWindow->setVisible(true);
            }
        };
        addAndMakeVisible(analyzerBtn);
    }

    void setProcessor(BZideProcessor* proc) { processorRef = proc; }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // EQ section has a gray background instead of black
        auto contentArea = getContentArea();
        g.setColour(juce::Colour(0xFF1A1A20));
        g.fillRect(contentArea);

        // --- Recessed panel header lambda ---
        auto drawSectionHeader = [&](int headerY, const juce::String& text) {
            int hdrH = 18;
            auto hdrArea = juce::Rectangle<int>(6, headerY, sectionWidth - 12, hdrH);
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(hdrArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(hdrArea.reduced(1).toFloat(), 2.0f);
            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(hdrArea.getY() + 1, (float)hdrArea.getX() + 2, (float)hdrArea.getRight() - 2);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(hdrArea.getBottom() - 2, (float)hdrArea.getX() + 2, (float)hdrArea.getRight() - 2);
            float cy = (float)(headerY + hdrH / 2);
            g.setColour(juce::Colour(0xFF444448));
            int textHalfW = (int)(text.length() * 3.5f) + 4;
            g.drawHorizontalLine((int)cy, (float)hdrArea.getX() + 4, (float)(sectionWidth / 2 - textHalfW));
            g.drawHorizontalLine((int)cy, (float)(sectionWidth / 2 + textHalfW), (float)hdrArea.getRight() - 4);
            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText(text, hdrArea, juce::Justification::centred);
        };

        // --- Divider lambda ---
        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawHorizontalLine(divY, 8.0f, (float)(sectionWidth - 8));
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(divY + 1, 8.0f, (float)(sectionWidth - 8));
        };

        // Vertical divider lambda
        auto drawVDiv = [&](int vx, int vTop, int vBot) {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawVerticalLine(vx, (float)vTop, (float)vBot);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawVerticalLine(vx + 1, (float)vTop, (float)vBot);
        };

        // Helper: draw internal structure for a band
        auto drawBandStructure = [&](const juce::Slider& gain, const juce::Slider& freq, const juce::Slider& q,
                                     const juce::TextButton& c1) {
            // Vertical line between GAIN and FREQ/Q
            int vx = gain.getRight() + 1;
            int vTop = gain.getY() - 2;
            int vBot = q.getBottom() + 2;
            drawVDiv(vx, vTop, vBot);

            // Horizontal line between FREQ and Q
            int hY = freq.getBottom() + 1;
            drawDiv(hY);

            // Horizontal line above curve buttons
            drawDiv(c1.getY() - 3);
        };

        // --- Draw headers, internal structure, and dividers ---
        drawSectionHeader(highHeaderY, "HIGH");
        drawBandStructure(eqHighGain, eqHighFreq, eqHighQ, highCurve1);
        drawDiv(highDivY);

        drawSectionHeader(midHeaderY, "MID");
        drawBandStructure(eqMidGain, eqMidFreq, eqMidQ, midCurve1);
        drawDiv(midDivY);

        drawSectionHeader(lowHeaderY, "LOW");
        drawBandStructure(eqLowGain, eqLowFreq, eqLowQ, lowCurve1);
        drawDiv(lowDivY);

        drawSectionHeader(filtersHeaderY, "FILTERS");

        // Divider below FILTERS + EQ PREVIEW header
        if (analyzerHeaderY > 0)
        {
            drawDiv(analyzerHeaderY - 4);
            drawSectionHeader(analyzerHeaderY, "EQ PREVIEW");
        }

        // Vertical divider between HPF and LPF
        if (eqHpf.getHeight() > 0)
        {
            int filterVx = (eqHpf.getRight() + eqLpf.getX()) / 2;
            drawVDiv(filterVx, eqHpf.getY() - 8, eqHpf.getBottom() + 2);
        }

        // --- Filter slope icons ---
        // HPF: high-pass slope icon (low cut)
        {
            float cx = (float)eqHpf.getBounds().getCentreX();
            float iy = (float)eqHpf.getY() - 6.0f;
            juce::Path hp;
            hp.startNewSubPath(cx - 7.0f, iy);
            hp.lineTo(cx - 2.0f, iy);
            hp.lineTo(cx + 2.0f, iy - 5.0f);
            hp.lineTo(cx + 7.0f, iy - 5.0f);
            g.setColour(juce::Colour(0xFF666670));
            g.strokePath(hp, juce::PathStrokeType(1.2f));
        }
        // LPF: low-pass slope icon (high cut)
        {
            float cx = (float)eqLpf.getBounds().getCentreX();
            float iy = (float)eqLpf.getY() - 6.0f;
            juce::Path lp;
            lp.startNewSubPath(cx - 7.0f, iy - 5.0f);
            lp.lineTo(cx - 2.0f, iy - 5.0f);
            lp.lineTo(cx + 2.0f, iy);
            lp.lineTo(cx + 7.0f, iy);
            g.setColour(juce::Colour(0xFF666670));
            g.strokePath(lp, juce::PathStrokeType(1.2f));
        }
    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());

        // Range marks helper
        // Draws "lo  LABEL  hi" on one line below the knob's text box
        auto drawRangeMarks = [&](const juce::Slider& s, const juce::String& lo, const juce::String& label, const juce::String& hi) {
            int markY = s.getBottom() - 6;
            int w = s.getWidth() + 16;
            int x = s.getX() - 8;
            g.setColour(juce::Colour(0xFFAAAAAA));
            g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
            g.drawText(lo, x, markY, w, 10, juce::Justification::centredLeft);
            g.drawText(hi, x, markY, w, 10, juce::Justification::centredRight);
            g.setColour(juce::Colour(0xFFCCCCCC));
            g.drawText(label, x, markY, w, 10, juce::Justification::centred);
        };

        // Simple labels above each knob — no range marks
        g.setColour(juce::Colour(0xFF999999));
        g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());

        labelAbove(g, eqHighGain, "GAIN");
        labelAbove(g, eqHighFreq, "FREQ");
        labelAbove(g, eqHighQ, "Q");

        labelAbove(g, eqMidGain, "GAIN");
        labelAbove(g, eqMidFreq, "FREQ");
        labelAbove(g, eqMidQ, "Q");

        labelAbove(g, eqLowGain, "GAIN");
        labelAbove(g, eqLowFreq, "FREQ");
        labelAbove(g, eqLowQ, "Q");

        g.setColour(juce::Colour(0xFF999999));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(g, eqHpf, "HPF");
        labelAbove(g, eqLpf, "LPF");

        // --- Curve type icons ---
        auto paintCurveIcons = [&](juce::TextButton& btn1, juce::TextButton& btn2, juce::TextButton& btn3) {
            // Button 1: Low shelf icon (flat left, rises on right)
            {
                auto b = btn1.getBounds().toFloat();
                float cx = b.getCentreX(), cy = b.getCentreY();
                juce::Path p;
                p.startNewSubPath(cx - 8.0f, cy + 2.0f);
                p.lineTo(cx - 2.0f, cy + 2.0f);
                p.lineTo(cx + 2.0f, cy - 3.0f);
                p.lineTo(cx + 8.0f, cy - 3.0f);
                g.setColour(btn1.getToggleState() ? juce::Colour(0xFFf97316) : juce::Colour(0xFF555558));
                g.strokePath(p, juce::PathStrokeType(1.4f));
            }
            // Button 2: Bell/peak icon (bump in middle)
            {
                auto b = btn2.getBounds().toFloat();
                float cx = b.getCentreX(), cy = b.getCentreY();
                juce::Path p;
                p.startNewSubPath(cx - 8.0f, cy + 2.0f);
                p.lineTo(cx - 4.0f, cy + 2.0f);
                p.quadraticTo(cx, cy - 6.0f, cx + 4.0f, cy + 2.0f);
                p.lineTo(cx + 8.0f, cy + 2.0f);
                g.setColour(btn2.getToggleState() ? juce::Colour(0xFFf97316) : juce::Colour(0xFF555558));
                g.strokePath(p, juce::PathStrokeType(1.4f));
            }
            // Button 3: High shelf icon (rises on left, flat right)
            {
                auto b = btn3.getBounds().toFloat();
                float cx = b.getCentreX(), cy = b.getCentreY();
                juce::Path p;
                p.startNewSubPath(cx - 8.0f, cy - 3.0f);
                p.lineTo(cx - 2.0f, cy - 3.0f);
                p.lineTo(cx + 2.0f, cy + 2.0f);
                p.lineTo(cx + 8.0f, cy + 2.0f);
                g.setColour(btn3.getToggleState() ? juce::Colour(0xFFf97316) : juce::Colour(0xFF555558));
                g.strokePath(p, juce::PathStrokeType(1.4f));
            }
        };

        paintCurveIcons(highCurve1, highCurve2, highCurve3);
        paintCurveIcons(midCurve1, midCurve2, midCurve3);
        paintCurveIcons(lowCurve1, lowCurve2, lowCurve3);
    }

    void resizeSectionContent() override
    {
        int bigKnob = 38;
        int smallKnob = 30;
        int pad = 16; // extra pad for LED ring clearance
        int y = getContentStartY() + 2;

        int gainW = bigKnob + pad;
        int freqW = smallKnob + pad;
        int totalW = gainW + freqW + 4;
        int startX = (sectionWidth - totalW) / 2;

        // Layout lambda for each band (HIGH, MID, LOW all identical)
        auto layoutBand = [&](int& headerY, int& divY,
                              juce::Slider& gain, juce::Slider& freq, juce::Slider& q,
                              juce::TextButton& curve1, juce::TextButton& curve2, juce::TextButton& curve3,
                              const juce::String& /*name*/)
        {
            headerY = y;
            y += 20; // header
            y += 16; // clearance for labels above knobs

            // FREQ small knob top-right
            int freqY = y;
            freq.setBounds(startX + gainW + 6, freqY, freqW, smallKnob + 22);

            // Q small knob bottom-right (with gap for horizontal divider + label)
            int qY = freqY + smallKnob + 22 + 18;
            q.setBounds(startX + gainW + 6, qY, freqW, smallKnob + 22);

            // GAIN big knob on left, spanning height from FREQ to Q bottom
            int gainTop = freqY;
            int gainBot = qY + smallKnob + 22;
            gain.setBounds(startX, gainTop, gainW, gainBot - gainTop);

            y = gainBot + 6;

            // Curve type buttons row — centered
            int curveBtnW = 36;
            int curveBtnH = 18;
            int curveGap = 2;
            int curveTotalW = curveBtnW * 3 + curveGap * 2;
            int curveX = (sectionWidth - curveTotalW) / 2;
            curve1.setBounds(curveX, y, curveBtnW, curveBtnH);
            curve2.setBounds(curveX + curveBtnW + curveGap, y, curveBtnW, curveBtnH);
            curve3.setBounds(curveX + (curveBtnW + curveGap) * 2, y, curveBtnW, curveBtnH);
            y += curveBtnH + 4;

            divY = y;
            y += 6;
        };

        // ---- BAND 1: HIGH ----
        layoutBand(highHeaderY, highDivY, eqHighGain, eqHighFreq, eqHighQ,
                   highCurve1, highCurve2, highCurve3, "HIGH");

        // ---- BAND 2: MID ----
        layoutBand(midHeaderY, midDivY, eqMidGain, eqMidFreq, eqMidQ,
                   midCurve1, midCurve2, midCurve3, "MID");

        // ---- BAND 3: LOW ----
        layoutBand(lowHeaderY, lowDivY, eqLowGain, eqLowFreq, eqLowQ,
                   lowCurve1, lowCurve2, lowCurve3, "LOW");

        // ---- BAND 4: FILTERS ----
        filtersHeaderY = y;
        y += 20;
        y += 16; // clearance for HPF/LPF labels

        int filterKnob = 34;
        int filterPad = 12;
        int fw = filterKnob + filterPad;
        int filterGap = 8;
        int filterTotalW = fw * 2 + filterGap;
        int filterStartX = (sectionWidth - filterTotalW) / 2;
        eqHpf.setBounds(filterStartX, y, fw, filterKnob + 22);
        eqLpf.setBounds(filterStartX + fw + filterGap, y, fw, filterKnob + 22);
        y += filterKnob + 22 + 8;

        // EQ PREVIEW header + Analyzer button
        analyzerHeaderY = y;
        y += 22; // header space
        int btnW = sectionWidth - 20;
        analyzerBtn.setBounds((sectionWidth - btnW) / 2, y, btnW, 22);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    juce::Slider eqHpf, eqLpf;
    juce::Slider eqLowFreq, eqLowGain, eqLowQ;
    juce::Slider eqMidFreq, eqMidGain, eqMidQ;
    juce::Slider eqHighFreq, eqHighGain, eqHighQ;

    // Curve type buttons
    juce::TextButton highCurve1 { "" }, highCurve2 { "" }, highCurve3 { "" };
    juce::TextButton midCurve1 { "" }, midCurve2 { "" }, midCurve3 { "" };
    juce::TextButton lowCurve1 { "" }, lowCurve2 { "" }, lowCurve3 { "" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<BA> eqBypassAtt;
    std::unique_ptr<SA> eqHpfAtt, eqLpfAtt;
    std::unique_ptr<SA> eqLowFreqAtt, eqLowGainAtt;
    std::unique_ptr<SA> eqMidFreqAtt, eqMidGainAtt, eqMidQAtt;
    std::unique_ptr<SA> eqHighFreqAtt, eqHighGainAtt, eqHighQAtt;
    std::unique_ptr<SA> eqLowQAtt;

    // Stored Y positions for paint pass
    int highHeaderY = 0, highDivY = 0;
    int midHeaderY = 0, midDivY = 0;
    int lowHeaderY = 0, lowDivY = 0;
    int filtersHeaderY = 0;
    int analyzerHeaderY = 0;

    // Analyzer
    BZideProcessor* processorRef = nullptr;
    juce::TextButton analyzerBtn;
    std::unique_ptr<AnalyzerWindow> analyzerWindow;
};

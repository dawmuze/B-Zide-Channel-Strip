#pragma once
#include "ChannelSection.h"

class PreSection : public ChannelSection
{
public:
    PreSection(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::PRE, "PREAMP", 140, true),
          apvtsRef(apvts)
    {
        // Bypass → base class ON button
        preBypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "pre_bypass", bypassBtn);

        // SATURATION knob — big
        setupKnob(preDrive, "%");
        preDriveAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pre_drive", preDrive);

        // HPF knob (uses pre_tone parameter)
        setupKnob(preHPF);
        preHPFAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pre_tone", preHPF);

        // LPF knob (display-only for now — no parameter yet)
        setupKnob(preLPF);
        preLPF.setRange(20.0, 20000.0, 1.0);
        preLPF.setValue(20000.0, juce::dontSendNotification);

        // INPUT GAIN / MIX / OUTPUT knobs
        setupKnob(preInput, " dB");
        preInputAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pre_input", preInput);
        setupKnob(preMix, "%");
        preMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pre_mix", preMix);
        setupKnob(preOutput, " dB");
        preOutputAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pre_output", preOutput);

        // 3 type buttons — ODD / EVEN / HEAVY
        for (auto* btn : { &oddBtn, &evenBtn, &heavyBtn })
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(1001);
            addAndMakeVisible(btn);
        }
        oddBtn.onClick = [this]() { if (oddBtn.getToggleState()) setTypeParam(0); };
        evenBtn.onClick = [this]() { if (evenBtn.getToggleState()) setTypeParam(1); };
        heavyBtn.onClick = [this]() { if (heavyBtn.getToggleState()) setTypeParam(2); };

        int initType = (int)*apvts.getRawParameterValue("pre_type");
        if (initType == 0) oddBtn.setToggleState(true, juce::dontSendNotification);
        else if (initType == 1) evenBtn.setToggleState(true, juce::dontSendNotification);
        else heavyBtn.setToggleState(true, juce::dontSendNotification);

        // Slope buttons — 6 / 12 / 18 dB per octave (visual, left column)
        for (auto* btn : { &slope6L, &slope12L, &slope18L })
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(1002);
            addAndMakeVisible(btn);
        }
        slope12L.setToggleState(true, juce::dontSendNotification);

        // Slope buttons — right column
        for (auto* btn : { &slope6R, &slope12R, &slope18R })
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(1003);
            addAndMakeVisible(btn);
        }
        slope12R.setToggleState(true, juce::dontSendNotification);

        // CRUNCH mode button (4th saturation mode)
        crunchBtn.setClickingTogglesState(true);
        crunchBtn.setRadioGroupId(1001);
        addAndMakeVisible(crunchBtn);
        crunchBtn.onClick = [this]() { if (crunchBtn.getToggleState()) setTypeParam(3); };

        // LOWRIDE button
        thumpBtn.setClickingTogglesState(true);
        addAndMakeVisible(thumpBtn);

        // 2 dB / 4 dB buttons
        for (auto* btn : { &db2Btn, &db4Btn })
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(1004);
            addAndMakeVisible(btn);
        }
        db2Btn.setToggleState(true, juce::dontSendNotification);
    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        float dL = 8.0f;
        float dR = (float)(sectionWidth - 8);

        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawHorizontalLine(divY, dL, dR);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(divY + 1, dL, dR);
        };

        // Recessed panel around SATURATION knob
        if (preDrive.getHeight() > 0)
        {
            auto satArea = juce::Rectangle<int>(6, preDrive.getY() - 20, sectionWidth - 12,
                                                 preDrive.getHeight() + 24);
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(satArea.toFloat(), 4.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(satArea.reduced(1).toFloat(), 3.0f);
            g.setColour(juce::Colour(0x20000000));
            g.drawHorizontalLine(satArea.getY() + 1, (float)satArea.getX() + 3, (float)satArea.getRight() - 3);
            g.setColour(juce::Colour(0x0CFFFFFF));
            g.drawHorizontalLine(satArea.getBottom() - 2, (float)satArea.getX() + 3, (float)satArea.getRight() - 3);
        }

        // Divider above ODD/EVEN/HEAVY/CRUNCH
        if (oddBtn.getHeight() > 0)
            drawDiv(oddBtn.getY() - 6);

        // Divider below ODD/EVEN/HEAVY/CRUNCH
        if (crunchBtn.getHeight() > 0)
            drawDiv(crunchBtn.getBottom() + 4);

        // "FILTERS" header — recessed panel with depth
        if (preHPF.getHeight() > 0)
        {
            int filtersY = crunchBtn.getBottom() + 8;
            int filtersH = 18;
            auto filtersArea = juce::Rectangle<int>(6, filtersY - 2, sectionWidth - 12, filtersH);

            // Inset shadow background
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(filtersArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(filtersArea.reduced(1).toFloat(), 2.0f);

            // Top shadow
            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(filtersArea.getY() + 1, (float)filtersArea.getX() + 2, (float)filtersArea.getRight() - 2);
            // Bottom highlight
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(filtersArea.getBottom() - 2, (float)filtersArea.getX() + 2, (float)filtersArea.getRight() - 2);

            // Decorative lines flanking text
            float cy = (float)(filtersY + 6);
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)cy, (float)filtersArea.getX() + 4, (float)(sectionWidth / 2 - 28));
            g.drawHorizontalLine((int)cy, (float)(sectionWidth / 2 + 28), (float)filtersArea.getRight() - 4);

            // Text
            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("FILTERS", filtersArea, juce::Justification::centred);

            // Divider line below the FILTERS label panel
            drawDiv(filtersArea.getBottom() + 2);
        }

        // Filter type icons (above each knob, below the labels)
        if (preHPF.getHeight() > 0)
        {
            int iconSize = 12;
            int iconY = preHPF.getY() - 22;
            int iconLeftX = preHPF.getX() + (preHPF.getWidth() - iconSize) / 2;
            int iconRightX = preLPF.getX() + (preLPF.getWidth() - iconSize) / 2;

            // HPF icon — rising slope
            g.setColour(juce::Colour(0xFFf97316));
            {
                juce::Path hpf;
                float ix = (float)iconLeftX, iy = (float)iconY;
                hpf.startNewSubPath(ix, iy + iconSize);
                hpf.lineTo(ix + iconSize * 0.5f, iy + iconSize);
                hpf.lineTo(ix + iconSize * 0.7f, iy + 3);
                hpf.lineTo(ix + iconSize, iy + 3);
                g.strokePath(hpf, juce::PathStrokeType(1.5f));
            }

            // LPF icon — falling slope
            {
                juce::Path lpf;
                float ix = (float)iconRightX, iy = (float)iconY;
                lpf.startNewSubPath(ix, iy + 3);
                lpf.lineTo(ix + iconSize * 0.3f, iy + 3);
                lpf.lineTo(ix + iconSize * 0.5f, iy + iconSize);
                lpf.lineTo(ix + iconSize, iy + iconSize);
                g.strokePath(lpf, juce::PathStrokeType(1.5f));
            }
        }

        // Divider below FILTERS header / above filter icons
        if (preHPF.getHeight() > 0)
            drawDiv(preHPF.getY() - 28);

        // Vertical divider between HPF (left) and LPF (right) columns
        if (preHPF.getHeight() > 0 && slope6L.getHeight() > 0)
        {
            int divX = sectionWidth / 2;
            int divTop = preHPF.getY() - 26;
            int divBot = slope18L.getBottom() + 2;
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawVerticalLine(divX, (float)divTop, (float)divBot);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawVerticalLine(divX + 1, (float)divTop, (float)divBot);
        }

        // Divider below filter knob / above slope buttons
        if (slope6L.getHeight() > 0)
            drawDiv(slope6L.getY() - 4);

        // Divider below slope buttons
        if (slope18L.getHeight() > 0)
            drawDiv(slope18L.getBottom() + 3);

        // "LOW END" header — same recessed panel style as SATURATION/FILTERS
        if (thumpBtn.getHeight() > 0)
        {
            int lowY = thumpBtn.getY() - 22;
            int lowH = 18;
            auto lowArea = juce::Rectangle<int>(6, lowY, sectionWidth - 12, lowH);

            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(lowArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(lowArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(lowArea.getY() + 1, (float)lowArea.getX() + 2, (float)lowArea.getRight() - 2);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(lowArea.getBottom() - 2, (float)lowArea.getX() + 2, (float)lowArea.getRight() - 2);

            float cy = (float)(lowY + lowH / 2);
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)cy, (float)lowArea.getX() + 4, (float)(sectionWidth / 2 - 30));
            g.drawHorizontalLine((int)cy, (float)(sectionWidth / 2 + 30), (float)lowArea.getRight() - 4);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("SUB CONTROL", lowArea, juce::Justification::centred);
        }

        // Divider below LOWRIDE / above 2dB/4dB
        if (thumpBtn.getHeight() > 0)
            drawDiv(thumpBtn.getBottom() + 3);

        // Divider below 2dB/4dB
        if (db2Btn.getHeight() > 0)
            drawDiv(db2Btn.getBottom() + 4);

        // "PARALLEL" header — uses saved Y position
        if (preMix.getHeight() > 0 && parHeaderY > 0)
        {
            int parY = parHeaderY;
            int parH = 18;
            auto parArea = juce::Rectangle<int>(6, parY, sectionWidth - 12, parH);

            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(parArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(parArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(parArea.getY() + 1, (float)parArea.getX() + 2, (float)parArea.getRight() - 2);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(parArea.getBottom() - 2, (float)parArea.getX() + 2, (float)parArea.getRight() - 2);

            float pcy = (float)(parY + parH / 2);
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)pcy, (float)parArea.getX() + 4, (float)(sectionWidth / 2 - 30));
            g.drawHorizontalLine((int)pcy, (float)(sectionWidth / 2 + 30), (float)parArea.getRight() - 4);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("PARALLEL", parArea, juce::Justification::centred);
        }

        // "GAIN STAGING" header — uses saved Y position
        if (preInput.getHeight() > 0 && gsHeaderY > 0)
        {
            int gsY = gsHeaderY;
            int gsH = 18;
            auto gsArea = juce::Rectangle<int>(6, gsY, sectionWidth - 12, gsH);

            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(gsArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(gsArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(gsArea.getY() + 1, (float)gsArea.getX() + 2, (float)gsArea.getRight() - 2);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(gsArea.getBottom() - 2, (float)gsArea.getX() + 2, (float)gsArea.getRight() - 2);

            float cy = (float)(gsY + gsH / 2);
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)cy, (float)gsArea.getX() + 4, (float)(sectionWidth / 2 - 40));
            g.drawHorizontalLine((int)cy, (float)(sectionWidth / 2 + 40), (float)gsArea.getRight() - 4);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("GAIN STAGING", gsArea, juce::Justification::centred);
        }
    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        // "SATURATION" label — recessed panel with depth (same style as FILTERS)
        {
            int satLabelY = preDrive.getY() - 20;
            int satLabelH = 18;
            auto satLabelArea = juce::Rectangle<int>(6, satLabelY, sectionWidth - 12, satLabelH);

            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(satLabelArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(satLabelArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(satLabelArea.getY() + 1, (float)satLabelArea.getX() + 2, (float)satLabelArea.getRight() - 2);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(satLabelArea.getBottom() - 2, (float)satLabelArea.getX() + 2, (float)satLabelArea.getRight() - 2);

            float cy = (float)(satLabelY + satLabelH / 2);
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)cy, (float)satLabelArea.getX() + 4, (float)(sectionWidth / 2 - 36));
            g.drawHorizontalLine((int)cy, (float)(sectionWidth / 2 + 36), (float)satLabelArea.getRight() - 4);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("SATURATION", satLabelArea, juce::Justification::centred);
        }

        // "0" and "100" marks
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::Font(juce::FontOptions(8.0f)));
        g.drawText("0", preDrive.getX() - 6, preDrive.getBottom() - 20, 16, 10, juce::Justification::centred);
        g.drawText("100", preDrive.getRight() - 10, preDrive.getBottom() - 20, 22, 10, juce::Justification::centred);

        // Filter knob labels — HPF / LPF above icons, above each knob
        if (preHPF.getHeight() > 0)
        {
            g.setColour(juce::Colour(0xFF888888));
            g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());
            g.drawText("HPF", preHPF.getX(), preHPF.getY() - 32, preHPF.getWidth(), 10, juce::Justification::centred);
            g.drawText("LPF", preLPF.getX(), preLPF.getY() - 32, preLPF.getWidth(), 10, juce::Justification::centred);
        }

        // GAIN STAGING knob labels
        if (preInput.getHeight() > 0)
        {
            g.setColour(juce::Colour(0xFF666666));
            g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());
            labelAbove(g, preInput, "INPUT");
            labelAbove(g, preOutput, "OUTPUT");
            labelAbove(g, preMix, "MIX");
        }
    }

    void resizeSectionContent() override
    {
        int bigKnob = 100;
        int filterKnob = 44;
        int typeBtnW = (sectionWidth - 24) / 3;
        int typeBtnH = 24;

        int y = getContentStartY();

        // SATURATION label + big knob
        y += 16;
        centerKnob(preDrive, y, bigKnob);
        y += bigKnob + 24;

        // ODD / EVEN / HEAVY / CRUNCH — vertical stack, centered
        int typeBtnFullW = 80;
        int typeX = (sectionWidth - typeBtnFullW) / 2;
        oddBtn.setBounds(typeX, y, typeBtnFullW, typeBtnH);
        y += typeBtnH + 1;
        evenBtn.setBounds(typeX, y, typeBtnFullW, typeBtnH);
        y += typeBtnH + 1;
        heavyBtn.setBounds(typeX, y, typeBtnFullW, typeBtnH);
        y += typeBtnH + 1;
        crunchBtn.setBounds(typeX, y, typeBtnFullW, typeBtnH);
        y += typeBtnH + 8;

        // FILTERS section header (painted, not a component)
        y += 16;

        // Filter icons (painted) + HPF/LPF labels (painted) + 2 knobs
        y += 36;
        centerKnobPair(preHPF, preLPF, y, filterKnob);
        y += filterKnob + 26;

        // Slope buttons: 6/12/18 left column + 6/12/18 right column
        int slopeBtnW = (sectionWidth - 28) / 2;
        int slopeBtnH = 18;
        int slopeGap = 1;
        int slopeX1 = 10;
        int slopeX2 = slopeX1 + slopeBtnW + 8;

        slope6L.setBounds(slopeX1, y, slopeBtnW, slopeBtnH);
        slope6R.setBounds(slopeX2, y, slopeBtnW, slopeBtnH);
        y += slopeBtnH + slopeGap;
        slope12L.setBounds(slopeX1, y, slopeBtnW, slopeBtnH);
        slope12R.setBounds(slopeX2, y, slopeBtnW, slopeBtnH);
        y += slopeBtnH + slopeGap;
        slope18L.setBounds(slopeX1, y, slopeBtnW, slopeBtnH);
        slope18R.setBounds(slopeX2, y, slopeBtnW, slopeBtnH);
        y += slopeBtnH + 6;

        // SUB CONTROL header (painted) + LOWRIDE button
        y += 18;
        int thumpW = sectionWidth - 24;
        thumpBtn.setBounds((sectionWidth - thumpW) / 2, y, thumpW, 20);
        y += 24;

        // 2 dB / 4 dB buttons — side by side
        int dbBtnW = (sectionWidth - 24) / 2;
        db2Btn.setBounds(10, y, dbBtnW, 20);
        db4Btn.setBounds(10 + dbBtnW + 4, y, dbBtnW, 20);
        y += 26;

        // GAIN STAGING header position (saved for painting)
        gsHeaderY = y + 4;
        y += 22; // header height
        int gsKnob = 36;
        y += 14; // label clearance
        centerKnobPair(preInput, preOutput, y, gsKnob);
        y += gsKnob + 24;

        // PARALLEL header position (saved for painting)
        parHeaderY = y + 4;
        y += 22; // header height
        y += 14; // label clearance
        int mixKnob = 90; // big knob for MIX — main parallel control
        centerKnob(preMix, y, mixKnob);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    juce::Slider preDrive, preHPF, preLPF;
    juce::Slider preInput, preMix, preOutput;

    // Type buttons
    juce::TextButton oddBtn    { "GRIT" };
    juce::TextButton evenBtn   { "SILK" };
    juce::TextButton heavyBtn  { "BLAZE" };
    juce::TextButton crunchBtn { "GRIND" };

    // Slope buttons (left/right filter columns)
    juce::TextButton slope6L  { "6" };
    juce::TextButton slope12L { "12" };
    juce::TextButton slope18L { "18" };
    juce::TextButton slope6R  { "6" };
    juce::TextButton slope12R { "12" };
    juce::TextButton slope18R { "18" };

    // LOWRIDE
    juce::TextButton thumpBtn { "LOWRIDE" };

    // dB boost
    juce::TextButton db2Btn { "2 dB" };
    juce::TextButton db4Btn { "4 dB" };

    int gsHeaderY = 0, parHeaderY = 0; // saved Y positions for painted headers

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preDriveAtt, preHPFAtt, preInputAtt, preMixAtt, preOutputAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preBypassAtt;

    void setTypeParam(int value)
    {
        if (auto* param = apvtsRef.getParameter("pre_type"))
            param->setValueNotifyingHost(param->convertTo0to1((float)value));
    }
};

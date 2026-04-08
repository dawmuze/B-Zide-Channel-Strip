#pragma once
#include "ChannelSection.h"
#include "MeterChannel.h"

class OutputSection : public ChannelSection
{
public:
    OutputSection(BZideProcessor& proc, juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::OUTPUT, "OUTPUT", 340, false),
          processor(proc)
    {
        // Dual VU Meters
        addAndMakeVisible(vuMeterIn);
        addAndMakeVisible(vuMeterOut);

        // Meter select buttons: IN, GR, OUT
        for (auto* b : { &meterSelIn, &meterSelGR, &meterSelOut })
        {
            b->setClickingTogglesState(true);
            b->setRadioGroupId(9001);
            b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF8B1515));
            addAndMakeVisible(b);
        }
        meterSelOut.setToggleState(true, juce::dontSendNotification);

        // Routing buttons: L, STEREO, R (top row) — M, MONO, S (bottom row)
        for (auto* b : { &routeL, &routeStereo, &routeR, &routeM, &routeMono, &routeS })
        {
            b->setClickingTogglesState(true);
            b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF8B1515));
            addAndMakeVisible(b);
        }
        routeStereo.setToggleState(true, juce::dontSendNotification);

        // Limiter button + attachment
        limiterBtn.setClickingTogglesState(true);
        limiterBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFDD2200));
        addAndMakeVisible(limiterBtn);
        limiterBtnAtt = std::make_unique<BA>(apvts, "out_limiter", limiterBtn);

        // Limiter threshold knob
        setupKnob(limiterThresh);
        limiterThreshAtt = std::make_unique<SA>(apvts, "out_limiter_thresh", limiterThresh);

        // Limiter LED
        addAndMakeVisible(limLED);

        // Phase invert buttons — one per channel
        for (auto* b : { &phaseLBtn, &phaseRBtn })
        {
            b->setClickingTogglesState(true);
            b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF8B1515));
            addAndMakeVisible(b);
        }

        // 4 Faders — SSL-style vertical
        auto setupFader = [this](juce::Slider& f) {
            f.setSliderStyle(juce::Slider::LinearVertical);
            f.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            f.setPaintingIsUnclipped(true); // thumb can paint outside bounds
            addAndMakeVisible(f);
        };
        // INPUT faders (display-only — show input level)
        setupFader(inFaderL);
        inFaderL.setEnabled(false); inFaderL.setRange(-60.0, 12.0, 0.1);
        setupFader(inFaderR);
        inFaderR.setEnabled(false); inFaderR.setRange(-60.0, 12.0, 0.1);
        // OUTPUT faders (L linked to APVTS, R mirrors L)
        setupFader(outFaderL);
        outFaderAtt = std::make_unique<SA>(apvts, "out_fader", outFaderL);
        setupFader(outFaderR);
        outFaderR.setRange(-60.0, 12.0, 0.1);
        outFaderL.onValueChange = [this]() {
            outFaderR.setValue(outFaderL.getValue(), juce::dontSendNotification);
        };

        // Output mode combo (hidden — driven by routing buttons)
        outMode.addItemList({"STEREO", "MONO", "M/S"}, 1);
        outModeAtt = std::make_unique<CA>(apvts, "out_mode", outMode);

        // Link button
        linkBtn.setClickingTogglesState(true);
        linkBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF8B1515));
        addAndMakeVisible(linkBtn);
    }

    juce::TextButton& getLimiterButton() { return limiterBtn; }
    LEDComponent& getLED() { return limLED; }

    void updateMeter()
    {
        float inTarget = (processor.inputLevelL.load() + processor.inputLevelR.load()) * 0.5f;
        float outTarget = (processor.outputLevelL.load() + processor.outputLevelR.load()) * 0.5f;

        // Envelope followers
        if (inTarget > inputMeterLevel)
            inputMeterLevel += (inTarget - inputMeterLevel) * 0.3f;
        else
            inputMeterLevel += (inTarget - inputMeterLevel) * 0.05f;

        if (outTarget > outputMeterLevel)
            outputMeterLevel += (outTarget - outputMeterLevel) * 0.3f;
        else
            outputMeterLevel += (outTarget - outputMeterLevel) * 0.05f;

        // Update input faders to show input level (display-only)
        inFaderL.setValue(processor.inputLevelL.load(), juce::dontSendNotification);
        inFaderR.setValue(processor.inputLevelR.load(), juce::dontSendNotification);

        // Mirror output fader R from L
        outFaderR.setValue(outFaderL.getValue(), juce::dontSendNotification);

        // GR smoothing
        float grTarget = processor.gainReduction.load();
        if (grTarget < grLevel)
            grLevel += (grTarget - grLevel) * 0.3f;
        else
            grLevel += (grTarget - grLevel) * 0.05f;

        // VU meter display based on selection
        if (meterSelIn.getToggleState())
        {
            vuMeterIn.setLevel(inTarget);
            vuMeterOut.setLevel(inTarget);
        }
        else if (meterSelGR.getToggleState())
        {
            vuMeterIn.setLevel(grLevel);
            vuMeterOut.setLevel(grLevel);
        }
        else
        {
            vuMeterIn.setLevel(inTarget);
            vuMeterOut.setLevel(outTarget);
        }

        repaint();
    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // Logo area
        paintLogoArea(g);

        // ── Divider lines between every section ──
        auto area = getContentArea();
        float dL = (float)(area.getX() + 4);
        float dR = (float)(area.getRight() - 4);
        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawHorizontalLine(divY, dL, dR);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(divY + 1, dL, dR);
        };

        if (vuMeterIn.getHeight() > 0)
        {
            // Below VU meters
            drawDiv(vuMeterIn.getBottom() + 3);
            // Below meter select buttons
            drawDiv(meterSelIn.getBottom() + 3);
            // Below logo area
            if (!logoAreaBounds.isEmpty())
                drawDiv(logoAreaBounds.getBottom() + 1);
            // Below routing/limiter row
            // Limiter section divider (above limiter at the bottom)
            drawDiv(limiterBtn.getY() - 6);
            // Below phase/utility buttons
            drawDiv(phaseLBtn.getBottom() + 3);
            // Vertical divider between INPUT and OUTPUT blocks
            if (!inputLedBounds.isEmpty() && !outputLedBounds.isEmpty())
            {
                int divX = (inFaderR.getRight() + outFaderL.getX()) / 2;
                int divTop = phaseLBtn.getBottom() + 8;
                int divBot = inputReadoutBounds.isEmpty() ? getHeight() - 30 : inputReadoutBounds.getBottom();
                g.setColour(juce::Colour(0xFF2A2A30));
                g.drawVerticalLine(divX, (float)divTop, (float)divBot);
                g.setColour(juce::Colour(0x10FFFFFF));
                g.drawVerticalLine(divX + 1, (float)divTop, (float)divBot);
            }
            // Below faders/LED meters (above readouts) — enough space for thumb overshoot
            if (!inputReadoutBounds.isEmpty())
                drawDiv(inputReadoutBounds.getY() - 2);
            // Below readouts (above link)
            if (!inputReadoutBounds.isEmpty())
                drawDiv(inputReadoutBounds.getBottom() + 3);
        }

        // LED Meter Strips — no built-in scale, we draw our own
        drawLEDMeterStrip(g, inputLedBounds, inputMeterLevel, 0);
        drawLEDMeterStrip(g, outputLedBounds, outputMeterLevel, 0);

        // 2-class scale marks like the KI-2A image
        // Inner marks (between faders): -90, -66, -42, -24, -12, -6, -1
        // Outer marks (outside): -Inf, 0, 12
        drawDualScale(g, inFaderL, inFaderR, inputLedBounds, true);   // INPUT block
        drawDualScale(g, outFaderL, outFaderR, outputLedBounds, false); // OUTPUT block

        // Fader scale removed — dB marks from LED meter are enough

        // Digital readouts
        drawDigitalReadout(g, inputReadoutBounds, inputMeterLevel);
        drawDigitalReadoutFader(g, outputReadoutBounds);

        // Labels above LED meters
        g.setColour(juce::Colour(kDimText));
        g.setFont(juce::Font(juce::FontOptions(8.5f)).boldened());
        if (!inputLedBounds.isEmpty())
        {
            // Labels span the full block (Fader L + LED + Fader R)
            int inBlockLeft = inFaderL.getX();
            int inBlockRight = inFaderR.getRight();
            g.drawText("INPUT", inBlockLeft, inputLedBounds.getY() - 14, inBlockRight - inBlockLeft, 12, juce::Justification::centred);
            int outBlockLeft = outFaderL.getX();
            int outBlockRight = outFaderR.getRight();
            g.drawText("OUTPUT", outBlockLeft, outputLedBounds.getY() - 14, outBlockRight - outBlockLeft, 12, juce::Justification::centred);
        }

    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        // Draw phase invert icons ON TOP of the phase buttons
        auto drawPhaseIcon = [&](const juce::TextButton& btn) {
            auto b = btn.getBounds().toFloat().reduced(3.0f);
            float cx = b.getCentreX();
            float cy = b.getCentreY();
            float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.35f;
            bool on = btn.getToggleState();

            juce::Colour iconCol = on ? juce::Colour(0xFFDD2200) : juce::Colour(0xFF888888);

            // Circle
            g.setColour(iconCol);
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);

            // Diagonal line through circle (phase invert symbol)
            float offset = r * 0.85f;
            g.drawLine(cx - offset, cy + offset, cx + offset, cy - offset, 1.5f);
        };

        drawPhaseIcon(phaseLBtn);
        drawPhaseIcon(phaseRBtn);
    }

    void resizeSectionContent() override
    {
        int contentW = sectionWidth - 6;
        int y = kHeaderHeight + 4;

        // Row 1: Dual VU Meters — big, edge to edge
        int meterGap = 4;
        int meterW = (contentW - meterGap) / 2;        // ~168px each
        int meterH = (int)(meterW * 0.6f);             // ~101px — slightly taller for presence
        int totalMeterW = meterW * 2 + meterGap;
        int meterStartX = (sectionWidth - totalMeterW) / 2;
        vuMeterIn.setBounds(meterStartX, y + 2, meterW, meterH);
        vuMeterOut.setBounds(meterStartX + meterW + meterGap, y + 2, meterW, meterH);
        y += meterH + 8;

        // Row 2: Meter select buttons
        int btnW = 36, btnH = 16;
        int selStartX = (sectionWidth - (btnW * 3 + 6)) / 2;
        meterSelIn.setBounds(selStartX, y, btnW, btnH);
        meterSelGR.setBounds(selStartX + btnW + 3, y, btnW, btnH);
        meterSelOut.setBounds(selStartX + btnW * 2 + 6, y, btnW, btnH);
        y += btnH + 6;

        // Row 3: Logo area (painted, no components) — bigger for the image
        logoAreaBounds = juce::Rectangle<int>(3, y, contentW, 80);
        y += 84;

        // Row 4: Routing buttons — centered, 2 rows of 3
        int rBtnW = 52, rBtnH = 18;
        int routeGap = 2;
        int routeTotalW = rBtnW * 3 + routeGap * 2;
        int routeX = (sectionWidth - routeTotalW) / 2;
        routeL.setBounds(routeX, y, rBtnW, rBtnH);
        routeStereo.setBounds(routeX + rBtnW + routeGap, y, rBtnW, rBtnH);
        routeR.setBounds(routeX + (rBtnW + routeGap) * 2, y, rBtnW, rBtnH);
        routeM.setBounds(routeX, y + rBtnH + routeGap, rBtnW, rBtnH);
        routeMono.setBounds(routeX + rBtnW + routeGap, y + rBtnH + routeGap, rBtnW, rBtnH);
        routeS.setBounds(routeX + (rBtnW + routeGap) * 2, y + rBtnH + routeGap, rBtnW, rBtnH);
        y += rBtnH * 2 + routeGap + 8;

        // Phase buttons will be positioned after we know fader positions
        int halfW = sectionWidth / 2;
        y += 4;

        // Divider
        y += 4;

        // Row 6: 4 Faders with LED Meters BETWEEN each pair
        // Layout: [FaderL | LED Meter | FaderR]  gap  [FaderL | LED Meter | FaderR]
        int thumbPad = 0; // no padding — thumb clamping in LookAndFeel keeps it visible
        int meterStripY = y + 16; // room for INPUT/OUTPUT labels
        int meterStripH = 220; // shorter to leave room for thumb overshoot
        int ledW = 22;
        int faderW = 32;
        int blockGap = 12; // gap between INPUT block and OUTPUT block
        int innerGap = 2;

        // Each block = faderW + innerGap + ledW + innerGap + faderW
        int blockW = faderW * 2 + ledW + innerGap * 2;
        // halfW already declared above

        // Phase buttons — both above INPUT block (L and R phase invert)
        int phBtnW = 22;
        int phBtnH = 16;
        int phGap = 4;
        int inCPhase = halfW / 2;
        phaseLBtn.setBounds(inCPhase - phBtnW - phGap / 2, meterStripY - 16, phBtnW, phBtnH);
        phaseRBtn.setBounds(inCPhase + phGap / 2, meterStripY - 16, phBtnW, phBtnH);

        // Faders exactly same bounds as LED meters — setPaintingIsUnclipped handles thumb overflow
        // INPUT block — centered in LEFT half
        int inCenterX = halfW / 2;
        int ix = inCenterX - blockW / 2;
        inFaderL.setBounds(ix, meterStripY, faderW, meterStripH);
        ix += faderW + innerGap;
        inputLedBounds = juce::Rectangle<int>(ix, meterStripY, ledW, meterStripH);
        ix += ledW + innerGap;
        inFaderR.setBounds(ix, meterStripY, faderW, meterStripH);

        // OUTPUT block — centered in RIGHT half
        int outCenterX = halfW + halfW / 2;
        int ox = outCenterX - blockW / 2;
        outFaderL.setBounds(ox, meterStripY, faderW, meterStripH);
        ox += faderW + innerGap;
        outputLedBounds = juce::Rectangle<int>(ox, meterStripY, ledW, meterStripH);
        ox += ledW + innerGap;
        outFaderR.setBounds(ox, meterStripY, faderW, meterStripH);

        // Digital readouts below each block — centered in each half
        int readoutY = meterStripY + meterStripH + 28; // extra space for thumb overshoot at bottom
        int readoutW = blockW + 10, readoutH = 16;
        inputReadoutBounds = juce::Rectangle<int>(inCenterX - readoutW / 2, readoutY, readoutW, readoutH);
        outputReadoutBounds = juce::Rectangle<int>(outCenterX - readoutW / 2, readoutY, readoutW, readoutH);

        // Row 7: Link button
        int linkY = readoutY + readoutH + 6;
        linkBtn.setBounds((sectionWidth - 60) / 2, linkY, 60, 16);

        // ── LIMITER SECTION — bottom of the output panel ──
        int limY = linkY + 24;

        // Limiter button — centered left
        int limBtnW = 70;
        limiterBtn.setBounds((sectionWidth / 2) - limBtnW - 4, limY, limBtnW, 24);
        limLED.setBounds((sectionWidth / 2) + limBtnW - 48, limY + 3, 18, 18);

        // Thresh knob — centered right
        limiterThresh.setBounds((sectionWidth / 2) + 4, limY - 10, 66, 66);
    }

private:
    BZideProcessor& processor;

    // Colors
    static constexpr juce::uint32 kBgColor     = 0xFF08080A;
    static constexpr juce::uint32 kDimText     = 0xFF666666;
    static constexpr juce::uint32 kAccent      = 0xFFDD2200;
    static constexpr juce::uint32 kGreen       = 0xFF44BB44;
    static constexpr juce::uint32 kLedRed      = 0xFFDD2200;
    static constexpr juce::uint32 kLedAmber    = 0xFFf59e0b;
    static constexpr juce::uint32 kLedCyan     = 0xFF00AADD;
    static constexpr juce::uint32 kLedOff      = 0xFF1A1A1E;
    static constexpr juce::uint32 kSlotBg      = 0xFF0A0A0E;
    static constexpr juce::uint32 kLogoPanel   = 0xFF0E0E12;

    // Meter state
    float inputMeterLevel  = -100.0f;
    float outputMeterLevel = -100.0f;
    float grLevel = 0.0f;

    // Layout rects
    juce::Rectangle<int> logoAreaBounds;
    juce::Rectangle<int> inputLedBounds;
    juce::Rectangle<int> outputLedBounds;
    juce::Rectangle<int> inputReadoutBounds;
    juce::Rectangle<int> outputReadoutBounds;

    // Components — Dual VU meters
    MeterChannel vuMeterIn, vuMeterOut;

    // Meter select buttons
    juce::TextButton meterSelIn  { "IN" };
    juce::TextButton meterSelGR  { "GR" };
    juce::TextButton meterSelOut { "OUT" };

    // Routing buttons
    juce::TextButton routeL       { "LEFT" };
    juce::TextButton routeStereo  { "STEREO" };
    juce::TextButton routeR       { "RIGHT" };
    juce::TextButton routeM       { "MUTE" };
    juce::TextButton routeMono    { "MONO" };
    juce::TextButton routeS       { "SOLO" };

    // Limiter
    juce::TextButton limiterBtn   { "LIMIT" };
    juce::Slider     limiterThresh;
    LEDComponent     limLED;

    // Phase invert — L and R (icon drawn in paint, no text)
    juce::TextButton phaseLBtn    { "" };
    juce::TextButton phaseRBtn    { "" };

    // 4 Faders: INPUT L/R (display-only) + OUTPUT L/R (linked to APVTS)
    juce::Slider inFaderL, inFaderR;
    juce::Slider outFaderL, outFaderR;

    // Output mode (hidden combo for APVTS)
    juce::ComboBox outMode;

    // Link button
    juce::TextButton linkBtn { "LINK" };

    // Attachments
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> outFaderAtt, outFaderRAtt, limiterThreshAtt;
    std::unique_ptr<BA> limiterBtnAtt;
    std::unique_ptr<CA> outModeAtt;

    // ---- LED Meter constants ----
    static constexpr int kNumLEDs = 28;

    float getLEDdB(int index) const
    {
        static constexpr float dbValues[kNumLEDs] = {
            -90.0f, -66.0f, -60.0f, -54.0f, -48.0f, -42.0f, -36.0f, -30.0f,
            -24.0f, -20.0f, -18.0f, -15.0f, -12.0f, -10.0f, -9.0f, -8.0f,
            -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f,
             2.0f,  4.0f,  8.0f,  12.0f
        };
        return dbValues[index];
    }

    juce::Colour getLEDColor(float db) const
    {
        if (db > -3.0f)  return juce::Colour(kLedRed);
        if (db > -12.0f) return juce::Colour(kLedAmber);
        return juce::Colour(kLedCyan);
    }

    // ---- Draw one LED meter strip ----
    // scaleSide: -1 = labels on left, 1 = labels on right, 0 = no labels
    void drawLEDMeterStrip(juce::Graphics& g, juce::Rectangle<int> bounds, float level, int scaleSide) const
    {
        if (bounds.isEmpty()) return;

        auto slot = bounds.toFloat();

        // Recessed slot background
        g.setColour(juce::Colour(kSlotBg));
        g.fillRoundedRectangle(slot, 3.0f);
        g.setColour(juce::Colour(0x30000000));
        g.drawRoundedRectangle(slot.reduced(0.5f), 3.0f, 1.0f);

        // LED circles
        float ledDiam = 5.0f;
        float padX = (slot.getWidth() - ledDiam) * 0.5f;
        float totalH = slot.getHeight() - 8.0f;
        float spacing = totalH / (float)(kNumLEDs - 1);
        float startY = slot.getY() + 4.0f;

        for (int i = 0; i < kNumLEDs; ++i)
        {
            int drawIndex = kNumLEDs - 1 - i;
            float ledDb = getLEDdB(drawIndex);
            float cy = startY + (float)i * spacing;
            float cx = slot.getX() + padX;

            bool lit = (level >= ledDb);

            if (lit)
            {
                juce::Colour col = getLEDColor(ledDb);
                g.setColour(col.withAlpha(0.15f));
                g.fillEllipse(cx - 2.0f, cy - 2.0f, ledDiam + 4.0f, ledDiam + 4.0f);
                g.setColour(col);
                g.fillEllipse(cx, cy, ledDiam, ledDiam);
                g.setColour(col.brighter(0.5f).withAlpha(0.6f));
                g.fillEllipse(cx + 1.5f, cy + 1.0f, 2.0f, 2.0f);
            }
            else
            {
                g.setColour(juce::Colour(kLedOff));
                g.fillEllipse(cx, cy, ledDiam, ledDiam);
            }
        }

        // dB scale labels
        if (scaleSide != 0)
        {
            g.setColour(juce::Colour(kDimText));
            g.setFont(juce::Font(juce::FontOptions(7.0f)));

            struct ScaleMark { float db; const char* label; };
            static constexpr ScaleMark marks[] = {
                { 12.0f, "12" }, { 0.0f, "0" }, { -6.0f, "-6" },
                { -12.0f, "-12" }, { -24.0f, "-24" }, { -42.0f, "-42" },
                { -66.0f, "-66" }, { -90.0f, "-Inf" }
            };

            for (auto& m : marks)
            {
                float norm = 0.0f;
                for (int i = 0; i < kNumLEDs; ++i)
                {
                    if (std::abs(getLEDdB(i) - m.db) < 0.5f)
                    {
                        int drawIdx = kNumLEDs - 1 - i;
                        norm = (float)drawIdx / (float)(kNumLEDs - 1);
                        break;
                    }
                }
                float my = startY + norm * totalH;

                if (scaleSide < 0)
                {
                    // Left side — tick mark then label
                    g.setColour(juce::Colour(0xFF333338));
                    g.drawHorizontalLine((int)my, slot.getX() - 4.0f, slot.getX() - 1.0f);
                    g.setColour(juce::Colour(kDimText));
                    g.drawText(m.label, (int)(slot.getX() - 30.0f), (int)(my - 4.0f), 24, 9, juce::Justification::centredRight);
                }
                else
                {
                    // Right side — tick mark then label
                    g.setColour(juce::Colour(0xFF333338));
                    g.drawHorizontalLine((int)my, slot.getRight() + 1.0f, slot.getRight() + 4.0f);
                    g.setColour(juce::Colour(kDimText));
                    g.drawText(m.label, (int)(slot.getRight() + 5.0f), (int)(my - 4.0f), 24, 9, juce::Justification::centredLeft);
                }
            }
        }
    }

    // ---- Dual scale: inner marks between faders, outer marks outside ----
    // Positions derived from actual LED dB values for perfect alignment
    void drawDualScale(juce::Graphics& g, const juce::Slider& faderL, const juce::Slider& faderR,
                       juce::Rectangle<int> ledBounds, bool isInput) const
    {
        if (ledBounds.isEmpty()) return;

        // Use fader's actual pixel position via getPositionOfValue()
        // Convert from fader-local coords to OutputSection coords
        auto dbToY = [&](float db) -> float {
            float localY = (float)faderL.getPositionOfValue(db);
            return localY + (float)faderL.getY(); // offset by fader's position in parent
        };

        // ── INNER marks (between the two faders) ──
        struct Mark { float db; const char* label; };
        static constexpr Mark inner[] = {
            { -1.0f, "-1" }, { -5.0f, "-5" }, { -12.0f, "-12" },
            { -24.0f, "-24" }, { -42.0f, "-42" }, { -60.0f, "-60" }
        };

        float innerLeft = (float)faderL.getRight() + 1.0f;
        float innerRight = (float)faderR.getX() - 1.0f;
        float innerCx = (innerLeft + innerRight) * 0.5f;

        for (auto& m : inner)
        {
            float my = dbToY(m.db);
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)my, innerLeft + 1.0f, innerRight - 1.0f);
            g.setColour(juce::Colour(0xFF888888));
            g.setFont(juce::Font(juce::FontOptions(6.0f)));
            g.drawText(m.label, (int)(innerCx - 10.0f), (int)(my - 8.0f), 20, 7, juce::Justification::centred);
        }

        // ── OUTER marks (outside the block) ──
        static constexpr Mark outer[] = {
            { 12.0f, "12" }, { 0.0f, "0" }, { -60.0f, "-Inf" }
        };

        for (auto& m : outer)
        {
            float my = dbToY(m.db);
            g.setFont(juce::Font(juce::FontOptions(7.5f)));

            if (isInput)
            {
                g.setColour(juce::Colour(0xFF444448));
                g.drawHorizontalLine((int)my, (float)faderL.getX() - 5.0f, (float)faderL.getX() - 1.0f);
                g.setColour(juce::Colour(kDimText));
                g.drawText(m.label, faderL.getX() - 28, (int)(my - 4.0f), 22, 9, juce::Justification::centredRight);
            }
            else
            {
                g.setColour(juce::Colour(0xFF444448));
                g.drawHorizontalLine((int)my, (float)faderR.getRight() + 1.0f, (float)faderR.getRight() + 5.0f);
                g.setColour(juce::Colour(kDimText));
                g.drawText(m.label, faderR.getRight() + 5, (int)(my - 4.0f), 24, 9, juce::Justification::centredLeft);
            }
        }
    }

    // ---- Shared dB scale between INPUT and OUTPUT ----
    void drawCenterScale(juce::Graphics& g) const
    {
        if (inputLedBounds.isEmpty() || outputLedBounds.isEmpty()) return;

        // Center X between the two blocks
        int centerX = (inFaderR.getRight() + outFaderL.getX()) / 2;

        float startY = (float)inputLedBounds.getY() + 4.0f;
        float totalH = (float)inputLedBounds.getHeight() - 8.0f;

        g.setColour(juce::Colour(kDimText));
        g.setFont(juce::Font(juce::FontOptions(7.5f)));

        struct ScaleMark { float db; const char* label; };
        static constexpr ScaleMark marks[] = {
            { 12.0f, "12" }, { 0.0f, "0" }, { -6.0f, "-6" },
            { -12.0f, "-12" }, { -24.0f, "-24" }, { -42.0f, "-42" },
            { -66.0f, "-66" }, { -90.0f, "-Inf" }
        };

        for (auto& m : marks)
        {
            float norm = 0.0f;
            for (int i = 0; i < kNumLEDs; ++i)
            {
                if (std::abs(getLEDdB(i) - m.db) < 0.5f)
                {
                    int drawIdx = kNumLEDs - 1 - i;
                    norm = (float)drawIdx / (float)(kNumLEDs - 1);
                    break;
                }
            }
            float my = startY + norm * totalH;

            // Draw centered between the two blocks
            g.drawText(m.label, centerX - 14, (int)(my - 4.0f), 28, 9, juce::Justification::centred);

            // Small tick marks on each side
            g.setColour(juce::Colour(0xFF333338));
            g.drawHorizontalLine((int)my, (float)(centerX - 16), (float)(centerX - 14));
            g.drawHorizontalLine((int)my, (float)(centerX + 14), (float)(centerX + 16));
            g.setColour(juce::Colour(kDimText));
        }
    }

    // ---- Logo area — DAWMUZE KI SERIES x Bzide rendered in code ----
    void paintLogoArea(juce::Graphics& g) const
    {
        if (logoAreaBounds.isEmpty()) return;

        // Dark panel background
        g.setColour(juce::Colour(kLogoPanel));
        g.fillRect(logoAreaBounds);

        auto area = logoAreaBounds.toFloat();
        float cx = area.getCentreX();
        int ty = logoAreaBounds.getY() + 6;

        // "DAWMUZE" — Menlo Bold, spaced letters, orange
        g.setColour(juce::Colour(0xFFf97316));
        g.setFont(juce::Font(juce::FontOptions("Menlo", 16.0f, juce::Font::bold)));
        g.drawText("D A W M U Z E", logoAreaBounds.getX(), ty, logoAreaBounds.getWidth(), 18, juce::Justification::centred);

        // Decorative lines flanking "KI SERIES"
        float lineY = (float)(ty + 22);
        float lineW = 60.0f;
        g.setColour(juce::Colour(0xFF555555));
        g.drawHorizontalLine((int)lineY, cx - lineW - 30.0f, cx - 30.0f);
        g.drawHorizontalLine((int)lineY, cx + 30.0f, cx + lineW + 30.0f);

        // "KI SERIES" — bold, smaller, white
        g.setColour(juce::Colour(0xFFDDDDDD));
        g.setFont(juce::Font(juce::FontOptions("Menlo", 10.0f, juce::Font::bold)));
        g.drawText("KI SERIES", logoAreaBounds.getX(), ty + 18, logoAreaBounds.getWidth(), 12, juce::Justification::centred);

        // Decorative lines below KI SERIES
        float lineY2 = (float)(ty + 33);
        g.setColour(juce::Colour(0xFF555555));
        g.drawHorizontalLine((int)lineY2, cx - lineW - 30.0f, cx - 30.0f);
        g.drawHorizontalLine((int)lineY2, cx + 30.0f, cx + lineW + 30.0f);

        // "x" small
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("x", logoAreaBounds.getX(), ty + 38, logoAreaBounds.getWidth(), 10, juce::Justification::centred);

        // "Bzide" — italic script style, larger
        g.setColour(juce::Colour(0xFFEEEEEE));
        g.setFont(juce::Font(juce::FontOptions(18.0f)).italicised().boldened());
        g.drawText("Bzide", logoAreaBounds.getX(), ty + 48, logoAreaBounds.getWidth(), 22, juce::Justification::centred);
    }

    // ---- Digital readout (level display) ----
    void drawDigitalReadout(juce::Graphics& g, juce::Rectangle<int> bounds, float levelDb) const
    {
        if (bounds.isEmpty()) return;
        auto r = bounds.toFloat();
        g.setColour(juce::Colour(kSlotBg));
        g.fillRoundedRectangle(r, 2.0f);
        g.setColour(juce::Colour(0xFF151518));
        g.drawRoundedRectangle(r, 2.0f, 0.5f);

        juce::String text;
        if (levelDb <= -90.0f)
            text = "-Inf";
        else
            text = juce::String(levelDb, 1) + " dB";

        g.setColour(juce::Colour(kLedCyan));
        g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
        g.drawText(text, bounds, juce::Justification::centred);
    }

    // ---- Digital readout (fader value) ----
    void drawDigitalReadoutFader(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        if (bounds.isEmpty()) return;
        auto r = bounds.toFloat();
        g.setColour(juce::Colour(kSlotBg));
        g.fillRoundedRectangle(r, 2.0f);
        g.setColour(juce::Colour(0xFF151518));
        g.drawRoundedRectangle(r, 2.0f, 0.5f);

        float val = outFaderL.getValue();
        juce::String text;
        if (val <= -96.0f)
            text = "-Inf";
        else
            text = juce::String(val, 1) + " dB";

        g.setColour(juce::Colour(kLedCyan));
        g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
        g.drawText(text, bounds, juce::Justification::centred);
    }

    // ---- Fader scale marks ----
    void drawFaderScale(juce::Graphics& g) const
    {
        if (outFaderL.getHeight() == 0) return;

        g.setColour(juce::Colour(kDimText));
        g.setFont(juce::Font(juce::FontOptions(7.5f)));

        auto fb = outFaderL.getBounds();
        float range = (float)(outFaderL.getMaximum() - outFaderL.getMinimum());

        float dbMarks[] = { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };
        for (float db : dbMarks)
        {
            float norm = (float)((db - outFaderL.getMinimum()) / range);
            float yPos = (float)fb.getBottom() - norm * (float)fb.getHeight();

            // Tick on left side of fader
            g.drawHorizontalLine((int)yPos, (float)(fb.getX() - 4), (float)(fb.getX() - 1));
            // Tick on right side of fader
            g.drawHorizontalLine((int)yPos, (float)(fb.getRight() + 1), (float)(fb.getRight() + 4));
        }
    }
};

#include "BZideEditor.h"

BZideEditor::BZideEditor(BZideProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);

    int totalWidth = kSectionWidth * 6 + kOutputWidth;
    setSize(totalWidth, kTotalHeight);
    startTimerHz(30);

    auto& apvts = processor.getAPVTS();

    // LED indicators
    addAndMakeVisible(preLED); addAndMakeVisible(eqLED); addAndMakeVisible(dsLED);
    addAndMakeVisible(compLED); addAndMakeVisible(gateLED); addAndMakeVisible(limLED);

    // ── PRE Section ──
    preBypass.setClickingTogglesState(true);
    addAndMakeVisible(preBypass);
    preBypassAtt = std::make_unique<ButtonAttachment>(apvts, "pre_bypass", preBypass);

    preType.addItemList({"ODD", "EVEN", "HEAVY"}, 1);
    addAndMakeVisible(preType);
    preTypeAtt = std::make_unique<ComboAttachment>(apvts, "pre_type", preType);

    setupRotaryKnob(preDrive, "%"); preDriveAtt = std::make_unique<SliderAttachment>(apvts, "pre_drive", preDrive);
    setupRotaryKnob(preTone); preToneAtt = std::make_unique<SliderAttachment>(apvts, "pre_tone", preTone);
    setupLabel(preDriveLabel, "DRIVE"); setupLabel(preToneLabel, "TONE");

    // ── EQ Section ──
    eqBypass.setClickingTogglesState(true);
    eqBypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentColor));
    addAndMakeVisible(eqBypass);
    eqBypassAtt = std::make_unique<ButtonAttachment>(apvts, "eq_bypass", eqBypass);

    setupRotaryKnob(eqHighGain); eqHighGainAtt = std::make_unique<SliderAttachment>(apvts, "eq_high_gain", eqHighGain);
    setupRotaryKnob(eqHighFreq); eqHighFreqAtt = std::make_unique<SliderAttachment>(apvts, "eq_high_freq", eqHighFreq);
    setupRotaryKnob(eqMidGain); eqMidGainAtt = std::make_unique<SliderAttachment>(apvts, "eq_mid_gain", eqMidGain);
    setupRotaryKnob(eqMidFreq); eqMidFreqAtt = std::make_unique<SliderAttachment>(apvts, "eq_mid_freq", eqMidFreq);
    setupRotaryKnob(eqMidQ); eqMidQAtt = std::make_unique<SliderAttachment>(apvts, "eq_mid_q", eqMidQ);
    setupRotaryKnob(eqLowGain); eqLowGainAtt = std::make_unique<SliderAttachment>(apvts, "eq_low_gain", eqLowGain);
    setupRotaryKnob(eqLowFreq); eqLowFreqAtt = std::make_unique<SliderAttachment>(apvts, "eq_low_freq", eqLowFreq);
    setupRotaryKnob(eqHpf); eqHpfAtt = std::make_unique<SliderAttachment>(apvts, "eq_hpf", eqHpf);
    setupRotaryKnob(eqLpf); eqLpfAtt = std::make_unique<SliderAttachment>(apvts, "eq_lpf", eqLpf);

    // ── DS² Section ──
    dsBypass.setClickingTogglesState(true);
    dsBypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentColor));
    addAndMakeVisible(dsBypass);
    dsBypassAtt = std::make_unique<ButtonAttachment>(apvts, "ds_bypass", dsBypass);

    setupRotaryKnob(dsFreq1); dsFreq1Att = std::make_unique<SliderAttachment>(apvts, "ds_freq1", dsFreq1);
    setupRotaryKnob(dsThresh1); dsThresh1Att = std::make_unique<SliderAttachment>(apvts, "ds_thresh1", dsThresh1);
    setupRotaryKnob(dsFreq2); dsFreq2Att = std::make_unique<SliderAttachment>(apvts, "ds_freq2", dsFreq2);
    setupRotaryKnob(dsThresh2); dsThresh2Att = std::make_unique<SliderAttachment>(apvts, "ds_thresh2", dsThresh2);

    // ── COMP Section ──
    compBypass.setClickingTogglesState(true);
    compBypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentColor));
    addAndMakeVisible(compBypass);
    compBypassAtt = std::make_unique<ButtonAttachment>(apvts, "comp_bypass", compBypass);

    compType.addItemList({"VCA", "FET", "OPT"}, 1);
    addAndMakeVisible(compType);
    compTypeAtt = std::make_unique<ComboAttachment>(apvts, "comp_type", compType);

    setupRotaryKnob(compThresh); compThreshAtt = std::make_unique<SliderAttachment>(apvts, "comp_threshold", compThresh);
    setupRotaryKnob(compRatio); compRatioAtt = std::make_unique<SliderAttachment>(apvts, "comp_ratio", compRatio);
    setupRotaryKnob(compAttack); compAttackAtt = std::make_unique<SliderAttachment>(apvts, "comp_attack", compAttack);
    setupRotaryKnob(compRelease); compReleaseAtt = std::make_unique<SliderAttachment>(apvts, "comp_release", compRelease);
    setupRotaryKnob(compMakeup); compMakeupAtt = std::make_unique<SliderAttachment>(apvts, "comp_makeup", compMakeup);
    setupRotaryKnob(compMix); compMixAtt = std::make_unique<SliderAttachment>(apvts, "comp_mix", compMix);

    // ── GATE Section ──
    gateBypass.setClickingTogglesState(true);
    gateBypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentColor));
    addAndMakeVisible(gateBypass);
    gateBypassAtt = std::make_unique<ButtonAttachment>(apvts, "gate_bypass", gateBypass);

    gateType.addItemList({"GATE", "EXP"}, 1);
    addAndMakeVisible(gateType);
    gateTypeAtt = std::make_unique<ComboAttachment>(apvts, "gate_type", gateType);

    setupRotaryKnob(gateThresh); gateThreshAtt = std::make_unique<SliderAttachment>(apvts, "gate_threshold", gateThresh);
    setupRotaryKnob(gateAtten); gateAttenAtt = std::make_unique<SliderAttachment>(apvts, "gate_atten", gateAtten);
    setupRotaryKnob(gateFloor); gateFloorAtt = std::make_unique<SliderAttachment>(apvts, "gate_floor", gateFloor);
    setupRotaryKnob(gateAttack); gateAttackAtt = std::make_unique<SliderAttachment>(apvts, "gate_attack", gateAttack);
    setupRotaryKnob(gateRelease); gateReleaseAtt = std::make_unique<SliderAttachment>(apvts, "gate_release", gateRelease);

    // ── OUTPUT Section ──
    outFader.setSliderStyle(juce::Slider::LinearVertical);
    outFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    outFader.setTextValueSuffix(" dB");
    addAndMakeVisible(outFader);
    outFaderAtt = std::make_unique<SliderAttachment>(apvts, "out_fader", outFader);

    limiterBtn.setClickingTogglesState(true);
    limiterBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentColor));
    addAndMakeVisible(limiterBtn);
    limiterBtnAtt = std::make_unique<ButtonAttachment>(apvts, "out_limiter", limiterBtn);

    setupRotaryKnob(limiterThresh); limiterThreshAtt = std::make_unique<SliderAttachment>(apvts, "out_limiter_thresh", limiterThresh);

    outMode.addItemList({"STEREO", "MONO", "M/S"}, 1);
    addAndMakeVisible(outMode);
    outModeAtt = std::make_unique<ComboAttachment>(apvts, "out_mode", outMode);
}

BZideEditor::~BZideEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void BZideEditor::timerCallback()
{
    // Sync LED states with buttons
    preLED.setOn(preBypass.getToggleState());
    eqLED.setOn(eqBypass.getToggleState());
    dsLED.setOn(dsBypass.getToggleState());
    compLED.setOn(compBypass.getToggleState());
    gateLED.setOn(gateBypass.getToggleState());
    limLED.setOn(limiterBtn.getToggleState());
    repaint();
}

void BZideEditor::setupRotaryKnob(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 13);
    s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFF999999));
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF0A0A0E));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    if (suffix.isNotEmpty()) s.setTextValueSuffix(suffix);
    addAndMakeVisible(s);
}

void BZideEditor::setupLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions(9.0f)));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colour(dimTextColor));
    addAndMakeVisible(l);
}

void BZideEditor::drawSectionHeader(juce::Graphics& g, int x, const juce::String& title)
{
    g.setColour(juce::Colour(headerColor));
    g.fillRect(x, 0, kSectionWidth, kHeaderHeight);
    g.setColour(juce::Colour(0xFFAAAAAA));
    g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
    g.drawText(title, x + 4, 0, kSectionWidth - 8, kHeaderHeight, juce::Justification::centredLeft);
}

// KI-2A exact LED function
static void drawLED(juce::Graphics& g, float cx, float cy, float r, bool on, juce::Colour color)
{
    if (on)
    {
        // Outer glow — large, very transparent
        g.setColour(color.withAlpha(0.12f));
        g.fillEllipse(cx - r * 2.5f, cy - r * 2.5f, r * 5.0f, r * 5.0f);
        // Solid LED
        g.setColour(color);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xFF2A2A30));
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }
}

void BZideEditor::paint(juce::Graphics& g)
{
    // Black background
    g.fillAll(juce::Colour(bgColor));

    int x = 0;

    // Section backgrounds + headers
    for (int i = 0; i < 6; ++i)
    {
        g.setColour(juce::Colour(sectionColor));
        g.fillRect(x, 0, kSectionWidth, kTotalHeight);
        x += kSectionWidth;
    }
    // Output section
    g.setColour(juce::Colour(sectionColor));
    g.fillRect(x, 0, kOutputWidth, kTotalHeight);

    // Headers
    x = 0;
    juce::StringArray headers = { "PRE", "EQ", "DS\xC2\xB2", "COMP", "GATE", "INSERT", "OUTPUT" };
    for (int i = 0; i < 6; ++i) { drawSectionHeader(g, x, headers[i]); x += kSectionWidth; }
    drawSectionHeader(g, x, headers[6]);

    // Separators
    g.setColour(juce::Colour(separatorColor));
    for (int i = 1; i <= 6; ++i)
        g.drawVerticalLine(i * kSectionWidth, 0.0f, (float)kTotalHeight);

    // ── Section dividers (painted under knobs is fine) ──

    // EQ dividers
    {
        int sx = kSectionWidth;
        auto drawDivider = [&](int divY) {
            g.setColour(juce::Colour(0xFF333338));
            g.drawHorizontalLine(divY, (float)(sx + 8), (float)(sx + kSectionWidth - 8));
        };
        drawDivider(eqHighGain.getBottom() + 6);
        drawDivider(eqMidQ.getBottom() + 6);
        drawDivider(eqLowGain.getBottom() + 6);
    }

    // DS² divider
    {
        int sx = 2 * kSectionWidth;
        g.setColour(juce::Colour(0xFF333338));
        g.drawHorizontalLine(dsThresh1.getBottom() + 6, (float)(sx + 8), (float)(sx + kSectionWidth - 8));
    }

    // COMP dividers
    {
        int sx = 3 * kSectionWidth;
        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF333338));
            g.drawHorizontalLine(divY, (float)(sx + 8), (float)(sx + kSectionWidth - 8));
        };
        drawDiv(compThresh.getBottom() + 4);
        drawDiv(compRatio.getBottom() + 4);
        drawDiv(compAttack.getBottom() + 4);
    }

    // GATE dividers
    {
        int sx = 4 * kSectionWidth;
        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF333338));
            g.drawHorizontalLine(divY, (float)(sx + 8), (float)(sx + kSectionWidth - 8));
        };
        drawDiv(gateThresh.getBottom() + 4);
        drawDiv(gateAtten.getBottom() + 4);
        drawDiv(gateFloor.getBottom() + 4);
    }

    // INSERT label (vertical)
    g.setColour(juce::Colour(dimTextColor));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    int insX = 5 * kSectionWidth;
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,
        (float)(insX + kSectionWidth / 2), (float)(kTotalHeight / 2)));
    g.drawText("No Insert", insX, kTotalHeight / 2 - 50, kSectionWidth, 100, juce::Justification::centred);
    g.restoreState();

    // OUTPUT meters
    int outX = 6 * kSectionWidth;
    int meterY = kHeaderHeight + 10;

    // IN meter
    drawMeter(g, juce::Rectangle<int>(outX + 15, meterY, 40, 200),
              (processor.inputLevelL.load() + processor.inputLevelR.load()) * 0.5f);
    g.setColour(juce::Colour(dimTextColor));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("IN", outX + 15, meterY + 202, 40, 12, juce::Justification::centred);

    // GR meter
    drawGRMeter(g, juce::Rectangle<int>(outX + 70, meterY, 30, 200), processor.gainReduction.load());
    g.drawText("GR", outX + 70, meterY + 202, 30, 12, juce::Justification::centred);

    // OUT meter
    drawMeter(g, juce::Rectangle<int>(outX + 115, meterY, 40, 200),
              (processor.outputLevelL.load() + processor.outputLevelR.load()) * 0.5f);
    g.drawText("OUT", outX + 115, meterY + 202, 40, 12, juce::Justification::centred);

    // Trial banner
    auto status = processor.getLicenseStatus();
    if (status != LicenseValidator::Status::Active)
    {
        juce::String text;
        juce::Colour color;
        switch (status)
        {
            case LicenseValidator::Status::Trial:
                text = "Trial: " + juce::String(processor.getTrialDaysRemaining()) + " days remaining";
                color = juce::Colour(0xFFf59e0b); break;
            case LicenseValidator::Status::Expired: text = "Trial Expired"; color = juce::Colour(accentColor); break;
            default: text = "Not Activated"; color = juce::Colour(dimTextColor); break;
        }
        auto banner = getLocalBounds().removeFromBottom(20);
        g.setColour(juce::Colour(0xDD000000));
        g.fillRect(banner);
        g.setColour(color);
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText(text, banner, juce::Justification::centred);
    }
}

void BZideEditor::paintOverChildren(juce::Graphics& g)
{
    // ── Knob labels — painted OVER children so LED dots don't cover them ──
    g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
    g.setColour(juce::Colour(dimTextColor));

    auto labelAbove = [&](const juce::Slider& s, const juce::String& text) {
        g.drawText(text, s.getX() - 10, s.getY() - 20, s.getWidth() + 20, 11, juce::Justification::centred);
    };
    auto labelBetween = [&](const juce::Slider& s1, const juce::Slider& s2,
                            const juce::String& t1, const juce::String& t2) {
        g.drawText(t1, s1.getX() - 10, s1.getY() - 20, s1.getWidth() + 20, 11, juce::Justification::centred);
        g.drawText(t2, s2.getX() - 10, s2.getY() - 20, s2.getWidth() + 20, 11, juce::Justification::centred);
    };

    // PRE
    labelAbove(preDrive, "DRIVE");
    labelAbove(preTone, "TONE");

    // EQ — with section titles
    {
        int sx = kSectionWidth;

        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        g.drawText("HIGH", sx, eqHighGain.getY() - 14, kSectionWidth, 10, juce::Justification::centred);
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());
        labelBetween(eqHighGain, eqHighFreq, "GAIN", "FREQ");

        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        g.drawText("MID", sx, eqMidGain.getY() - 14, kSectionWidth, 10, juce::Justification::centred);
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());
        labelBetween(eqMidGain, eqMidFreq, "GAIN", "FREQ");
        labelAbove(eqMidQ, "Q");

        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        g.drawText("LOW", sx, eqLowGain.getY() - 14, kSectionWidth, 10, juce::Justification::centred);
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());
        labelBetween(eqLowGain, eqLowFreq, "GAIN", "FREQ");

        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        g.drawText("FILTERS", sx, eqHpf.getY() - 14, kSectionWidth, 10, juce::Justification::centred);
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());
        labelBetween(eqHpf, eqLpf, "HPF", "LPF");
    }

    g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
    g.setColour(juce::Colour(dimTextColor));

    // DS²
    {
        int sx = 2 * kSectionWidth;

        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("BAND 1", sx, dsFreq1.getY() - 16, kSectionWidth, 12, juce::Justification::centred);
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(dsFreq1, "FREQ");
        labelAbove(dsThresh1, "THRESH");

        g.setColour(juce::Colour(0xFFDD6600));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("BAND 2", sx, dsFreq2.getY() - 16, kSectionWidth, 12, juce::Justification::centred);
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(dsFreq2, "FREQ");
        labelAbove(dsThresh2, "THRESH");
    }

    // COMP
    {
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(compThresh, "THRESH");
        labelAbove(compRatio, "RATIO");
        labelBetween(compAttack, compRelease, "ATK", "REL");
        labelBetween(compMakeup, compMix, "GAIN", "MIX");
    }

    // GATE
    {
        g.setColour(juce::Colour(dimTextColor));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(gateThresh, "THRESH");
        labelAbove(gateAtten, "ATTEN");
        labelAbove(gateFloor, "FLOOR");
        labelBetween(gateAttack, gateRelease, "ATK", "REL");
    }

    // ── KI-2A LED bombillas — painted OVER everything so they're always visible ──
    juce::Colour ledColor(0xFFDD2200);
    float ledR = 3.5f;

    struct LedInfo { juce::TextButton& btn; };
    LedInfo leds[] = { {preBypass}, {eqBypass}, {dsBypass}, {compBypass}, {gateBypass}, {limiterBtn} };

    for (auto& led : leds)
    {
        auto b = led.btn.getBounds();
        float cx = (float)(b.getRight() + 10);
        float cy = (float)(b.getCentreY());
        bool on = led.btn.getToggleState();

        if (on)
        {
            g.setColour(ledColor.withAlpha(0.12f));
            g.fillEllipse(cx - ledR * 2.5f, cy - ledR * 2.5f, ledR * 5.0f, ledR * 5.0f);
            g.setColour(ledColor);
            g.fillEllipse(cx - ledR, cy - ledR, ledR * 2.0f, ledR * 2.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.fillEllipse(cx - ledR, cy - ledR, ledR * 2.0f, ledR * 2.0f);
        }
    }
}

void BZideEditor::resized()
{
    int knobSize = 52;
    int knobSmall = 42;
    auto centerKnob = [&](juce::Slider& s, int sx, int sy, int size) {
        s.setBounds(sx + (kSectionWidth - size) / 2, sy, size, size + 14);
    };
    auto centerKnobPair = [&](juce::Slider& s1, juce::Slider& s2, int sx, int sy, int size) {
        int gap = 4;
        int totalW = size * 2 + gap;
        int startX = sx + (kSectionWidth - totalW) / 2;
        s1.setBounds(startX, sy, size, size + 14);
        s2.setBounds(startX + size + gap, sy, size, size + 14);
    };

    int ledW = 18;

    // ── PRE Section (x=0) ──
    int x = 0, y = kHeaderHeight + 8;
    int labelGap = 24; // space for label above each knob (must clear LED dots)
    preLED.setBounds(x + kSectionWidth - ledW - 2, 5, ledW, ledW);
    preBypass.setBounds(x + 8, y, kSectionWidth - 24, 22); y += 28;
    preType.setBounds(x + 8, y, kSectionWidth - 16, 22); y += 34;
    y += labelGap;
    centerKnob(preDrive, x, y, knobSize); y += knobSize + 22;
    y += labelGap;
    centerKnob(preTone, x, y, knobSize);

    // ── EQ Section ──
    x = kSectionWidth; y = kHeaderHeight + 8;
    eqLED.setBounds(x + kSectionWidth - ledW - 2, 5, ledW, ledW);
    eqBypass.setBounds(x + 8, y, kSectionWidth - 24, 22); y += 28;
    y += labelGap + 12; // extra for "HIGH" label
    centerKnobPair(eqHighGain, eqHighFreq, x, y, knobSmall); y += knobSmall + labelGap + 12;
    centerKnobPair(eqMidGain, eqMidFreq, x, y, knobSmall); y += knobSmall + labelGap + 4;
    centerKnob(eqMidQ, x, y, knobSmall); y += knobSmall + labelGap + 12;
    centerKnobPair(eqLowGain, eqLowFreq, x, y, knobSmall); y += knobSmall + labelGap + 4;
    centerKnobPair(eqHpf, eqLpf, x, y, knobSmall);

    // ── DS² Section ──
    x = 2 * kSectionWidth; y = kHeaderHeight + 8;
    dsLED.setBounds(x + kSectionWidth - ledW - 2, 5, ledW, ledW);
    dsBypass.setBounds(x + 8, y, kSectionWidth - 24, 22); y += 32;
    y += labelGap + 4;
    centerKnob(dsFreq1, x, y, knobSize); y += knobSize + labelGap + 4;
    centerKnob(dsThresh1, x, y, knobSize); y += knobSize + labelGap + 10;
    centerKnob(dsFreq2, x, y, knobSize); y += knobSize + labelGap + 4;
    centerKnob(dsThresh2, x, y, knobSize);

    // ── COMP Section ──
    x = 3 * kSectionWidth; y = kHeaderHeight + 8;
    compLED.setBounds(x + kSectionWidth - ledW - 2, 5, ledW, ledW);
    compBypass.setBounds(x + 8, y, kSectionWidth - 24, 22); y += 26;
    compType.setBounds(x + 8, y, kSectionWidth - 16, 22); y += 28;
    y += labelGap;
    centerKnob(compThresh, x, y, knobSize); y += knobSize + labelGap + 2;
    centerKnob(compRatio, x, y, knobSmall); y += knobSmall + labelGap + 2;
    centerKnobPair(compAttack, compRelease, x, y, knobSmall); y += knobSmall + labelGap + 2;
    centerKnobPair(compMakeup, compMix, x, y, knobSmall);

    // ── GATE Section ──
    x = 4 * kSectionWidth; y = kHeaderHeight + 8;
    gateLED.setBounds(x + kSectionWidth - ledW - 2, 5, ledW, ledW);
    gateBypass.setBounds(x + 8, y, kSectionWidth - 24, 22); y += 26;
    gateType.setBounds(x + 8, y, kSectionWidth - 16, 22); y += 30;
    y += labelGap;
    centerKnob(gateThresh, x, y, knobSize); y += knobSize + labelGap + 2;
    centerKnob(gateAtten, x, y, knobSmall); y += knobSmall + labelGap + 2;
    centerKnob(gateFloor, x, y, knobSmall); y += knobSmall + labelGap + 2;
    centerKnobPair(gateAttack, gateRelease, x, y, knobSmall);

    // ── INSERT Section — empty ──

    // ── OUTPUT Section ──
    x = 6 * kSectionWidth; y = kHeaderHeight + 220;
    outFader.setBounds(x + (kOutputWidth - 60) / 2, y, 60, 200); y += 210;
    limiterBtn.setBounds(x + 15, y, 70, 22);
    limLED.setBounds(x + 88, y + 2, ledW, ledW);
    limiterThresh.setBounds(x + 108, y - 4, 80, 30); y += 30;
    outMode.setBounds(x + 15, y, kOutputWidth - 30, 22);
}

void BZideEditor::drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float levelDb)
{
    g.setColour(juce::Colour(0xFF0A0A0E));
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    float norm = juce::jmap(juce::jlimit(-60.0f, 6.0f, levelDb), -60.0f, 6.0f, 0.0f, 1.0f);
    int barH = (int)(norm * bounds.getHeight());
    if (barH > 0)
    {
        auto bar = bounds.withTop(bounds.getBottom() - barH);
        g.setColour(norm > 0.9f ? juce::Colour(accentColor) : juce::Colour(greenColor));
        g.fillRoundedRectangle(bar.toFloat(), 2.0f);
    }
}

void BZideEditor::drawGRMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float grDb)
{
    g.setColour(juce::Colour(0xFF0A0A0E));
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    float norm = juce::jlimit(0.0f, 1.0f, std::abs(grDb) / 20.0f);
    int barH = (int)(norm * bounds.getHeight());
    if (barH > 0)
    {
        auto bar = bounds.withHeight(barH);
        g.setColour(juce::Colour(0xFFf59e0b));
        g.fillRoundedRectangle(bar.toFloat(), 2.0f);
    }
}

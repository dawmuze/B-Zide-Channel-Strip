#include "BZideEditor.h"

BZideEditor::BZideEditor(BZideProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);

    int totalWidth = kSectionWidth * 6 + kOutputWidth;
    setSize(totalWidth, kTotalHeight);
    startTimerHz(30);

    auto& apvts = processor.getAPVTS();

    // ── PRE Section ──
    preBypass.setClickingTogglesState(true);
    preBypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentColor));
    addAndMakeVisible(preBypass);
    preBypassAtt = std::make_unique<ButtonAttachment>(apvts, "pre_bypass", preBypass);

    preType.addItemList({"ODD", "EVEN", "HEAVY"}, 1);
    addAndMakeVisible(preType);
    preTypeAtt = std::make_unique<ComboAttachment>(apvts, "pre_type", preType);

    setupRotaryKnob(preDrive); preDriveAtt = std::make_unique<SliderAttachment>(apvts, "pre_drive", preDrive);
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

void BZideEditor::timerCallback() { repaint(); }

void BZideEditor::setupRotaryKnob(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
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

void BZideEditor::resized()
{
    int knobSize = 56;
    int knobSmall = 44;

    // ── PRE Section (x=0) ──
    int x = 0, y = kHeaderHeight + 8;
    preBypass.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 30;
    preType.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 36;
    preDrive.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 20;
    preDriveLabel.setBounds(x, y, kSectionWidth, 12); y += 18;
    preTone.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 20;
    preToneLabel.setBounds(x, y, kSectionWidth, 12);

    // ── EQ Section (x=120) ──
    x = kSectionWidth; y = kHeaderHeight + 8;
    eqBypass.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 32;
    eqHighGain.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    eqHighFreq.setBounds(x + 64, y, knobSmall, knobSmall + 14); y += knobSmall + 18;
    eqMidGain.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    eqMidFreq.setBounds(x + 64, y, knobSmall, knobSmall + 14); y += knobSmall + 18;
    eqMidQ.setBounds(x + 35, y, knobSmall, knobSmall + 14); y += knobSmall + 18;
    eqLowGain.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    eqLowFreq.setBounds(x + 64, y, knobSmall, knobSmall + 14); y += knobSmall + 18;
    eqHpf.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    eqLpf.setBounds(x + 64, y, knobSmall, knobSmall + 14);

    // ── DS² Section (x=240) ──
    x = 2 * kSectionWidth; y = kHeaderHeight + 8;
    dsBypass.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 36;
    dsFreq1.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 20;
    dsThresh1.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 30;
    dsFreq2.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 20;
    dsThresh2.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16);

    // ── COMP Section (x=360) ──
    x = 3 * kSectionWidth; y = kHeaderHeight + 8;
    compBypass.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 28;
    compType.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 30;
    compThresh.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 16;
    compRatio.setBounds(x + (kSectionWidth - knobSmall) / 2, y, knobSmall, knobSmall + 14); y += knobSmall + 14;
    compAttack.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    compRelease.setBounds(x + 64, y, knobSmall, knobSmall + 14); y += knobSmall + 14;
    compMakeup.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    compMix.setBounds(x + 64, y, knobSmall, knobSmall + 14);

    // ── GATE Section (x=480) ──
    x = 4 * kSectionWidth; y = kHeaderHeight + 8;
    gateBypass.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 28;
    gateType.setBounds(x + 10, y, kSectionWidth - 20, 22); y += 32;
    gateThresh.setBounds(x + (kSectionWidth - knobSize) / 2, y, knobSize, knobSize + 16); y += knobSize + 16;
    gateAtten.setBounds(x + (kSectionWidth - knobSmall) / 2, y, knobSmall, knobSmall + 14); y += knobSmall + 14;
    gateFloor.setBounds(x + (kSectionWidth - knobSmall) / 2, y, knobSmall, knobSmall + 14); y += knobSmall + 14;
    gateAttack.setBounds(x + 10, y, knobSmall, knobSmall + 14);
    gateRelease.setBounds(x + 64, y, knobSmall, knobSmall + 14);

    // ── INSERT Section (x=600) — empty ──

    // ── OUTPUT Section (x=720) ──
    x = 6 * kSectionWidth; y = kHeaderHeight + 230;
    outFader.setBounds(x + 60, y, 60, 200); y += 210;
    limiterBtn.setBounds(x + 15, y, 70, 22);
    limiterThresh.setBounds(x + 90, y - 6, 80, 34); y += 30;
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

#include "BZideEditor.h"

BZideEditor::BZideEditor(BZideProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(kTotalWidth, kTotalHeight);
    startTimerHz(30); // 30fps meter refresh
}

BZideEditor::~BZideEditor()
{
    stopTimer();
}

void BZideEditor::timerCallback()
{
    repaint(); // Refresh meters
}

void BZideEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(Colors::bg));

    auto& apvts = processor.getAPVTS();
    int x = 0;

    // ══════════════════════════════════════════════════════════════════
    //  PRE Section
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kSectionWidth, kTotalHeight);
        bool bypassed = *apvts.getRawParameterValue("pre_bypass") > 0.5f;
        drawSection(g, bounds, "PRE", bypassed);

        int cy = kHeaderHeight + 20;

        // ST / DUO / MS buttons
        auto typeNames = juce::StringArray{ "ST", "DUO", "MS" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 10, cy + i * 28, kSectionWidth - 20, 22);
            drawButton(g, btnRect, typeNames[i], i == 0);
        }
        cy += 100;

        // SATURATION label
        g.setColour(juce::Colour(Colors::accent));
        g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
        g.drawText("SATURATION", bounds.withY(cy).withHeight(16), juce::Justification::centred);
        cy += 24;

        // ODD / EVEN / HEAVY buttons
        int preType = static_cast<int>(*apvts.getRawParameterValue("pre_type"));
        auto satNames = juce::StringArray{ "ODD", "EVEN", "HEAVY" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 10, cy + i * 26, kSectionWidth - 20, 20);
            drawButton(g, btnRect, satNames[i], i == preType);
        }
        cy += 90;

        // Drive knob
        float drive = *apvts.getRawParameterValue("pre_drive");
        drawKnob(g, x + kSectionWidth / 2, cy, 28, drive, 0.0f, 100.0f, "DRIVE");
        cy += 80;

        // Tone knob
        float tone = *apvts.getRawParameterValue("pre_tone");
        drawKnob(g, x + kSectionWidth / 2, cy, 28, tone, -100.0f, 100.0f, "TONE");

        x += kSectionWidth;
    }

    // ══════════════════════════════════════════════════════════════════
    //  EQ Section
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kSectionWidth, kTotalHeight);
        bool bypassed = *apvts.getRawParameterValue("eq_bypass") > 0.5f;
        drawSection(g, bounds, "EQ", bypassed);

        int cy = kHeaderHeight + 20;

        // HIGH band
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText("HIGH", bounds.withY(cy).withHeight(14), juce::Justification::centred);
        cy += 16;
        float hiGain = *apvts.getRawParameterValue("eq_high_gain");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, hiGain, -18.0f, 18.0f, "GAIN");
        cy += 70;

        // Freq + Q readout
        float hiFreq = *apvts.getRawParameterValue("eq_high_freq");
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText(juce::String((int)hiFreq) + " Hz", bounds.withY(cy).withHeight(12), juce::Justification::centred);
        cy += 30;

        // MID band
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText("MID", bounds.withY(cy).withHeight(14), juce::Justification::centred);
        cy += 16;
        float midGain = *apvts.getRawParameterValue("eq_mid_gain");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, midGain, -18.0f, 18.0f, "GAIN");
        cy += 70;

        float midFreq = *apvts.getRawParameterValue("eq_mid_freq");
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText(juce::String((int)midFreq) + " Hz  Q:" +
                   juce::String(*apvts.getRawParameterValue("eq_mid_q"), 1),
                   bounds.withY(cy).withHeight(12), juce::Justification::centred);
        cy += 30;

        // LOW band
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText("LOW", bounds.withY(cy).withHeight(14), juce::Justification::centred);
        cy += 16;
        float lowGain = *apvts.getRawParameterValue("eq_low_gain");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, lowGain, -18.0f, 18.0f, "GAIN");
        cy += 70;

        // HPF / LPF
        float hpf = *apvts.getRawParameterValue("eq_hpf");
        float lpf = *apvts.getRawParameterValue("eq_lpf");
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("HPF " + juce::String((int)hpf), bounds.withY(cy).withHeight(12).withTrimmedRight(kSectionWidth / 2), juce::Justification::centred);
        g.drawText("LPF " + juce::String((int)lpf / 1000.0f, 1) + "k", bounds.withY(cy).withHeight(12).withTrimmedLeft(kSectionWidth / 2), juce::Justification::centred);

        x += kSectionWidth;
    }

    // ══════════════════════════════════════════════════════════════════
    //  DS² Section
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kSectionWidth, kTotalHeight);
        bool bypassed = *apvts.getRawParameterValue("ds_bypass") > 0.5f;
        drawSection(g, bounds, "DS\xC2\xB2", bypassed); // DS²

        int cy = kHeaderHeight + 30;

        // Band 1
        g.setColour(juce::Colour(Colors::accent));
        g.setFont(juce::Font(juce::FontOptions(13.0f)).boldened());
        g.drawText("1", bounds.withY(cy).withHeight(20), juce::Justification::centred);
        cy += 24;

        float f1 = *apvts.getRawParameterValue("ds_freq1");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, f1, 2000.0f, 12000.0f, "FREQ");
        cy += 70;

        float t1 = *apvts.getRawParameterValue("ds_thresh1");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, t1, -60.0f, 0.0f, "THRESH");
        cy += 90;

        // Band 2
        g.setColour(juce::Colour(Colors::accent));
        g.setFont(juce::Font(juce::FontOptions(13.0f)).boldened());
        g.drawText("2", bounds.withY(cy).withHeight(20), juce::Justification::centred);
        cy += 24;

        float f2 = *apvts.getRawParameterValue("ds_freq2");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, f2, 4000.0f, 20000.0f, "FREQ");
        cy += 70;

        float t2 = *apvts.getRawParameterValue("ds_thresh2");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 22, t2, -60.0f, 0.0f, "THRESH");

        x += kSectionWidth;
    }

    // ══════════════════════════════════════════════════════════════════
    //  COMP Section
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kSectionWidth, kTotalHeight);
        bool bypassed = *apvts.getRawParameterValue("comp_bypass") > 0.5f;
        drawSection(g, bounds, "COMP", bypassed);

        int cy = kHeaderHeight + 20;

        // ST / DUO / MS
        auto modeNames = juce::StringArray{ "ST", "DUO", "MS" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 10, cy + i * 28, kSectionWidth - 20, 22);
            drawButton(g, btnRect, modeNames[i], i == 0);
        }
        cy += 100;

        // VCA / FET / OPT
        int compType = static_cast<int>(*apvts.getRawParameterValue("comp_type"));
        auto typeNames = juce::StringArray{ "VCA", "FET", "OPT" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 10 + i * 30, cy, 28, 20);
            drawButton(g, btnRect, typeNames[i], i == compType);
        }
        cy += 34;

        // THRESHOLD
        float thresh = *apvts.getRawParameterValue("comp_threshold");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 24, thresh, -50.0f, 0.0f, "THRESHOLD");
        cy += 70;

        // GR meter
        float gr = processor.gainReduction.load();
        auto grRect = juce::Rectangle<int>(x + 20, cy, kSectionWidth - 40, 60);
        drawGRMeter(g, grRect, gr);
        cy += 70;

        // RATIO
        float ratio = *apvts.getRawParameterValue("comp_ratio");
        g.setColour(juce::Colour(Colors::knobFg));
        g.setFont(juce::Font(juce::FontOptions(18.0f)).boldened());
        g.drawText(juce::String(ratio, 1) + ":1", bounds.withY(cy).withHeight(24), juce::Justification::centred);
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("RATIO", bounds.withY(cy + 24).withHeight(12), juce::Justification::centred);
        cy += 46;

        // ATTACK
        float attack = *apvts.getRawParameterValue("comp_attack");
        drawKnob(g, x + kSectionWidth / 2, cy + 18, 18, attack, 0.1f, 150.0f, "ATTACK");
        cy += 50;

        // RELEASE
        float release = *apvts.getRawParameterValue("comp_release");
        drawKnob(g, x + kSectionWidth / 2, cy + 18, 18, release, 10.0f, 1000.0f, "RELEASE");
        cy += 52;

        // MIX / OUTPUT
        float mix = *apvts.getRawParameterValue("comp_mix");
        float output = *apvts.getRawParameterValue("comp_output");
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("MIX " + juce::String((int)mix) + "%", bounds.withY(cy).withHeight(12).withTrimmedRight(kSectionWidth / 2), juce::Justification::centred);
        g.drawText("OUT " + juce::String(output, 1) + "dB", bounds.withY(cy).withHeight(12).withTrimmedLeft(kSectionWidth / 2), juce::Justification::centred);

        x += kSectionWidth;
    }

    // ══════════════════════════════════════════════════════════════════
    //  GATE Section
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kSectionWidth, kTotalHeight);
        bool bypassed = *apvts.getRawParameterValue("gate_bypass") > 0.5f;
        drawSection(g, bounds, "GATE", bypassed);

        int cy = kHeaderHeight + 20;

        // ST / DUO / MS
        auto modeNames = juce::StringArray{ "ST", "DUO", "MS" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 10, cy + i * 28, kSectionWidth - 20, 22);
            drawButton(g, btnRect, modeNames[i], i == 0);
        }
        cy += 100;

        // GATE / EXP
        int gateType = static_cast<int>(*apvts.getRawParameterValue("gate_type"));
        drawButton(g, juce::Rectangle<int>(x + 10, cy, 42, 22), "GATE", gateType == 0);
        drawButton(g, juce::Rectangle<int>(x + 56, cy, 42, 22), "EXP", gateType == 1);
        cy += 36;

        // THRESHOLD
        float thresh = *apvts.getRawParameterValue("gate_threshold");
        drawKnob(g, x + kSectionWidth / 2, cy + 24, 24, thresh, -80.0f, 0.0f, "THRESHOLD");
        cy += 70;

        // ATTEN
        float atten = *apvts.getRawParameterValue("gate_atten");
        drawKnob(g, x + kSectionWidth / 2, cy + 18, 18, atten, -60.0f, 0.0f, "ATTEN");
        cy += 52;

        // FLOOR
        float floor = *apvts.getRawParameterValue("gate_floor");
        drawKnob(g, x + kSectionWidth / 2, cy + 18, 18, floor, -60.0f, 0.0f, "FLOOR");
        cy += 52;

        // CLOSE
        drawKnob(g, x + kSectionWidth / 2, cy + 18, 18, -48.0f, -60.0f, 0.0f, "CLOSE");
        cy += 52;

        // ATK / REL
        float atk = *apvts.getRawParameterValue("gate_attack");
        float rel = *apvts.getRawParameterValue("gate_release");
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("ATK " + juce::String(atk, 1) + "ms", bounds.withY(cy).withHeight(12), juce::Justification::centred);
        cy += 14;
        g.drawText("REL " + juce::String((int)rel) + "ms", bounds.withY(cy).withHeight(12), juce::Justification::centred);

        x += kSectionWidth;
    }

    // ══════════════════════════════════════════════════════════════════
    //  INSERT Section (placeholder)
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kSectionWidth, kTotalHeight);
        drawSection(g, bounds, "INSERT", false);

        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));

        // Vertical "INSERT" text
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,
            (float)(x + kSectionWidth / 2), (float)(kTotalHeight / 2)));
        g.drawText("No Insert", juce::Rectangle<int>(x, kTotalHeight / 2 - 50, kSectionWidth, 100),
                   juce::Justification::centred);
        g.restoreState();

        x += kSectionWidth;
    }

    // ══════════════════════════════════════════════════════════════════
    //  OUTPUT Section (meters + fader + limiter)
    // ══════════════════════════════════════════════════════════════════
    {
        auto bounds = juce::Rectangle<int>(x, 0, kOutputWidth, kTotalHeight);

        // Background
        g.setColour(juce::Colour(Colors::sectionBg));
        g.fillRect(bounds);

        // Header
        g.setColour(juce::Colour(0xFF333333));
        g.fillRect(bounds.withHeight(kHeaderHeight));
        g.setColour(juce::Colour(Colors::headerText));
        g.setFont(juce::Font(juce::FontOptions(12.0f)).boldened());
        g.drawText("OUTPUT", bounds.withHeight(kHeaderHeight), juce::Justification::centred);

        int cy = kHeaderHeight + 10;

        // VU Meters area — IN and OUT
        int meterWidth = 50;
        int meterHeight = 200;
        int meterX1 = x + 20;
        int meterX2 = x + kOutputWidth - 70;

        // IN meter
        float inL = processor.inputLevelL.load();
        float inR = processor.inputLevelR.load();
        drawMeter(g, juce::Rectangle<int>(meterX1, cy, meterWidth, meterHeight), (inL + inR) * 0.5f);
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("IN", juce::Rectangle<int>(meterX1, cy + meterHeight + 2, meterWidth, 12), juce::Justification::centred);

        // GR meter (center)
        float gr = processor.gainReduction.load();
        int grX = x + kOutputWidth / 2 - 15;
        drawGRMeter(g, juce::Rectangle<int>(grX, cy, 30, meterHeight), gr);
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("GR", juce::Rectangle<int>(grX, cy + meterHeight + 2, 30, 12), juce::Justification::centred);

        // OUT meter
        float outL = processor.outputLevelL.load();
        float outR = processor.outputLevelR.load();
        drawMeter(g, juce::Rectangle<int>(meterX2, cy, meterWidth, meterHeight), (outL + outR) * 0.5f);
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("OUT", juce::Rectangle<int>(meterX2, cy + meterHeight + 2, meterWidth, 12), juce::Justification::centred);

        cy += meterHeight + 20;

        // IN / GR / OUT selector buttons
        auto btnNames = juce::StringArray{ "IN", "GR", "OUT" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 20 + i * 50, cy, 44, 20);
            drawButton(g, btnRect, btnNames[i], i == 0);
        }
        cy += 30;

        // Fader readout
        float fader = *processor.getAPVTS().getRawParameterValue("out_fader");
        g.setColour(juce::Colour(Colors::knobFg));
        g.setFont(juce::Font(juce::FontOptions(16.0f)).boldened());
        g.drawText(juce::String(fader, 1) + " dB", bounds.withY(cy).withHeight(24), juce::Justification::centred);
        cy += 30;

        // FADER visual (simple vertical bar)
        {
            auto faderBounds = juce::Rectangle<int>(x + kOutputWidth / 2 - 8, cy, 16, 100);
            g.setColour(juce::Colour(0xFF444444));
            g.fillRoundedRectangle(faderBounds.toFloat(), 4.0f);
            float norm = juce::jmap(fader, -60.0f, 12.0f, 0.0f, 1.0f);
            int knobY = cy + (int)((1.0f - norm) * 90.0f);
            g.setColour(juce::Colour(Colors::accent));
            g.fillRoundedRectangle((float)(x + kOutputWidth / 2 - 14), (float)knobY, 28.0f, 10.0f, 3.0f);
        }
        cy += 110;

        // Limiter
        bool limOn = *processor.getAPVTS().getRawParameterValue("out_limiter") > 0.5f;
        drawButton(g, juce::Rectangle<int>(x + 20, cy, 60, 22), "LIMIT", limOn);
        float limThresh = *processor.getAPVTS().getRawParameterValue("out_limiter_thresh");
        g.setColour(juce::Colour(Colors::dimText));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText(juce::String(limThresh, 1) + " dB", juce::Rectangle<int>(x + 85, cy, 80, 22), juce::Justification::centredLeft);
        cy += 32;

        // Stereo / Mono / M-S
        int outMode = static_cast<int>(*processor.getAPVTS().getRawParameterValue("out_mode"));
        auto outModes = juce::StringArray{ "STEREO", "MONO", "M/S" };
        for (int i = 0; i < 3; ++i)
        {
            auto btnRect = juce::Rectangle<int>(x + 10 + i * 55, cy, 50, 20);
            drawButton(g, btnRect, outModes[i], i == outMode);
        }
    }

    // Separators between sections
    g.setColour(juce::Colour(Colors::separator));
    for (int i = 1; i <= 6; ++i)
    {
        int sx = i * kSectionWidth;
        if (i == 6) sx = 6 * kSectionWidth; // Before output
        g.drawVerticalLine(sx, 0.0f, (float)kTotalHeight);
    }

    // Trial banner
    drawTrialBanner(g);
}

void BZideEditor::resized() {}

// ═══ HELPERS ═══

void BZideEditor::drawSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                               const juce::String& title, bool bypassed)
{
    // Section background
    g.setColour(juce::Colour(Colors::sectionBg));
    g.fillRect(bounds);

    // Header
    g.setColour(juce::Colour(Colors::headerBg));
    g.fillRect(bounds.withHeight(kHeaderHeight));

    // Title
    g.setColour(bypassed ? juce::Colour(Colors::bypassBtn) : juce::Colour(Colors::headerText));
    g.setFont(juce::Font(juce::FontOptions(12.0f)).boldened());
    g.drawText(title, bounds.withHeight(kHeaderHeight).reduced(4, 0), juce::Justification::centredLeft);

    // Bypass indicator
    if (bypassed)
    {
        g.setColour(juce::Colour(Colors::bypassBtn));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("OFF", bounds.withHeight(kHeaderHeight).reduced(4, 0), juce::Justification::centredRight);
    }
}

void BZideEditor::drawKnob(juce::Graphics& g, int cx, int cy, int radius,
                            float value, float min, float max, const juce::String& label,
                            const juce::String& unit)
{
    float norm = (max > min) ? (value - min) / (max - min) : 0.0f;
    float angle = juce::jmap(norm, -2.4f, 2.4f); // ~270 degree sweep

    // Knob body
    g.setColour(juce::Colour(0xFF3a3a3a));
    g.fillEllipse((float)(cx - radius), (float)(cy - radius), (float)(radius * 2), (float)(radius * 2));

    // Knob border
    g.setColour(juce::Colour(0xFF555555));
    g.drawEllipse((float)(cx - radius), (float)(cy - radius), (float)(radius * 2), (float)(radius * 2), 1.5f);

    // Pointer line
    float lineLen = (float)radius * 0.7f;
    float endX = cx + std::sin(angle) * lineLen;
    float endY = cy - std::cos(angle) * lineLen;
    g.setColour(juce::Colour(Colors::knobFg));
    g.drawLine((float)cx, (float)cy, endX, endY, 2.0f);

    // Value text
    g.setColour(juce::Colour(Colors::dimText));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    juce::String valStr;
    if (std::abs(value) >= 1000.0f)
        valStr = juce::String(value / 1000.0f, 1) + "k";
    else if (std::abs(value) >= 100.0f)
        valStr = juce::String((int)value);
    else
        valStr = juce::String(value, 1);
    g.drawText(valStr + unit, juce::Rectangle<int>(cx - 30, cy + radius + 2, 60, 12), juce::Justification::centred);

    // Label
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(label, juce::Rectangle<int>(cx - 30, cy + radius + 14, 60, 10), juce::Justification::centred);
}

void BZideEditor::drawButton(juce::Graphics& g, juce::Rectangle<int> bounds,
                              const juce::String& text, bool active)
{
    g.setColour(active ? juce::Colour(Colors::activeBtn) : juce::Colour(0xFF3a3a3a));
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    g.setColour(active ? juce::Colours::white : juce::Colour(Colors::dimText));
    g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
    g.drawText(text, bounds, juce::Justification::centred);
}

void BZideEditor::drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds,
                             float levelDb, float minDb, float maxDb)
{
    // Background
    g.setColour(juce::Colour(0xFF111111));
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    // Level bar
    float norm = juce::jmap(juce::jlimit(minDb, maxDb, levelDb), minDb, maxDb, 0.0f, 1.0f);
    int barH = (int)(norm * bounds.getHeight());

    if (barH > 0)
    {
        auto barBounds = bounds.withTop(bounds.getBottom() - barH);
        // Green to yellow to red gradient
        float redStart = juce::jmap(0.0f, minDb, maxDb, 0.0f, 1.0f); // 0 dB mark
        if (norm > redStart)
        {
            // Red zone
            g.setColour(juce::Colour(Colors::meterRed));
            g.fillRoundedRectangle(barBounds.toFloat(), 2.0f);
        }
        else
        {
            g.setColour(juce::Colour(Colors::meterGreen));
            g.fillRoundedRectangle(barBounds.toFloat(), 2.0f);
        }
    }

    // dB marks
    g.setColour(juce::Colour(0xFF444444));
    g.setFont(juce::Font(juce::FontOptions(7.0f)));
    float dbMarks[] = { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };
    for (float db : dbMarks)
    {
        float y = juce::jmap(db, maxDb, minDb, (float)bounds.getY(), (float)bounds.getBottom());
        g.drawHorizontalLine((int)y, (float)bounds.getX(), (float)bounds.getRight());
    }
}

void BZideEditor::drawGRMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float grDb)
{
    // Background
    g.setColour(juce::Colour(0xFF111111));
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    // GR bar (goes down from top)
    float norm = juce::jlimit(0.0f, 1.0f, std::abs(grDb) / 20.0f);
    int barH = (int)(norm * bounds.getHeight());

    if (barH > 0)
    {
        auto barBounds = bounds.withHeight(barH);
        g.setColour(juce::Colour(Colors::accent));
        g.fillRoundedRectangle(barBounds.toFloat(), 2.0f);
    }

    // GR value
    g.setColour(juce::Colour(Colors::dimText));
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(juce::String(grDb, 1), bounds.withHeight(12).withY(bounds.getBottom() + 2), juce::Justification::centred);
}

void BZideEditor::drawTrialBanner(juce::Graphics& g)
{
    auto status = processor.getLicenseStatus();
    if (status == LicenseValidator::Status::Active) return;

    juce::String text;
    juce::Colour color;

    switch (status)
    {
        case LicenseValidator::Status::Trial:
            text = "Trial: " + juce::String(processor.getTrialDaysRemaining()) + " days remaining";
            color = juce::Colour(Colors::accent);
            break;
        case LicenseValidator::Status::Expired:
            text = "Trial Expired";
            color = juce::Colour(Colors::meterRed);
            break;
        case LicenseValidator::Status::Locked:
            text = "Connection Required";
            color = juce::Colour(Colors::meterRed);
            break;
        default:
            text = "Not Activated";
            color = juce::Colour(Colors::dimText);
            break;
    }

    auto banner = juce::Rectangle<int>(0, kTotalHeight - 20, kTotalWidth, 20);
    g.setColour(juce::Colour(0xDD000000));
    g.fillRect(banner);
    g.setColour(color);
    g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
    g.drawText(text, banner, juce::Justification::centred);
}

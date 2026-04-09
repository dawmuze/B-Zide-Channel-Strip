#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "../BZideProcessor.h"

class LimiterPanel : public juce::Component,
                     private juce::Timer
{
public:
    LimiterPanel(BZideProcessor& proc)
        : processor(proc),
          apvts(proc.getAPVTS())
    {
        setSize(420, 220);

        // ── ON/OFF button ──
        limiterBtn.setButtonText("LIMIT");
        limiterBtn.setClickingTogglesState(true);
        limiterBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A22));
        limiterBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFDD2200));
        limiterBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        limiterBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF888888));
        addAndMakeVisible(limiterBtn);
        limiterBtnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "out_limiter", limiterBtn);

        // ── Ceiling fader (vertical) ──
        ceilingFader.setSliderStyle(juce::Slider::LinearVertical);
        ceilingFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        ceilingFader.setColour(juce::Slider::trackColourId, juce::Colour(0xFFDD2200));
        ceilingFader.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFCCCCCC));
        ceilingFader.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF1A1A22));
        addAndMakeVisible(ceilingFader);
        ceilingAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "out_limiter_thresh", ceilingFader);

        // ── Mode buttons (visual only for now) ──
        const char* modeNames[] = { "STD", "SOFT", "TRUE" };
        for (int i = 0; i < 3; ++i)
        {
            modeBtns[i].setButtonText(modeNames[i]);
            modeBtns[i].setClickingTogglesState(true);
            modeBtns[i].setRadioGroupId(7777);
            modeBtns[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A22));
            modeBtns[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF333340));
            modeBtns[i].setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFf97316));
            modeBtns[i].setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF666666));
            addAndMakeVisible(modeBtns[i]);
        }
        modeBtns[0].setToggleState(true, juce::dontSendNotification);

        startTimerHz(30);
    }

    ~LimiterPanel() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Dark background
        g.fillAll(juce::Colour(0xFF0A0A10));

        // Subtle border
        g.setColour(juce::Colour(0xFF2A2A35));
        g.drawRect(bounds, 1.0f);

        // ── Section dividers ──
        // Vertical divider after ON/OFF column
        float divX1 = 90.0f;
        g.setColour(juce::Colour(0xFF2A2A30));
        g.drawVerticalLine((int)divX1, 8.0f, bounds.getHeight() - 30.0f);

        // Vertical divider after ceiling/GR section
        float divX2 = 260.0f;
        g.drawVerticalLine((int)divX2, 8.0f, bounds.getHeight() - 30.0f);

        // ── LED indicator (limiter active) ──
        {
            float ledX = 45.0f - 5.0f;
            float ledY = 110.0f;
            bool active = limiterBtn.getToggleState() && smoothedGR < -0.5f;

            if (active)
            {
                // Glow
                g.setColour(juce::Colour(0xFFDD2200).withAlpha(0.2f));
                g.fillEllipse(ledX - 4.0f, ledY - 4.0f, 18.0f, 18.0f);
                g.setColour(juce::Colour(0xFFDD2200));
            }
            else
            {
                g.setColour(juce::Colour(0xFF2A2A30));
            }
            g.fillEllipse(ledX, ledY, 10.0f, 10.0f);
            g.setColour(juce::Colour(0xFF444444));
            g.drawEllipse(ledX, ledY, 10.0f, 10.0f, 0.5f);
        }

        // ── GR Meter (LED dots) ──
        paintGRMeter(g);

        // ── Ceiling label ──
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("CEILING", 110, 6, 60, 14, juce::Justification::centred);

        // ── GR label ──
        g.drawText("GR", 190, 6, 50, 14, juce::Justification::centred);

        // ── dB scale marks next to the fader ──
        paintFaderScale(g);

        // ── Digital ceiling readout ──
        {
            auto readoutBounds = juce::Rectangle<float>(275.0f, 30.0f, 130.0f, 28.0f);
            g.setColour(juce::Colour(0xFF0A0A0E));
            g.fillRoundedRectangle(readoutBounds, 3.0f);
            g.setColour(juce::Colour(0xFF1A1A22));
            g.drawRoundedRectangle(readoutBounds, 3.0f, 1.0f);

            float val = ceilingFader.getValue();
            juce::String text = "Ceiling: " + juce::String(val, 1) + " dB";
            g.setColour(juce::Colour(0xFF00AADD));
            g.setFont(juce::Font(juce::FontOptions(12.0f)).boldened());
            g.drawText(text, readoutBounds.toNearestInt(), juce::Justification::centred);
        }

        // ── GR readout ──
        {
            auto grReadout = juce::Rectangle<float>(275.0f, 68.0f, 130.0f, 22.0f);
            g.setColour(juce::Colour(0xFF0A0A0E));
            g.fillRoundedRectangle(grReadout, 3.0f);
            g.setColour(juce::Colour(0xFF1A1A22));
            g.drawRoundedRectangle(grReadout, 3.0f, 1.0f);

            juce::String grText;
            if (smoothedGR > -0.1f)
                grText = "GR: 0.0 dB";
            else
                grText = "GR: " + juce::String(smoothedGR, 1) + " dB";
            g.setColour(juce::Colour(0xFFf59e0b));
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText(grText, grReadout.toNearestInt(), juce::Justification::centred);
        }

        // ── Mode label ──
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("MODE", 275, 100, 130, 14, juce::Justification::centred);

        // ── Bottom branding ──
        g.setColour(juce::Colour(0xFF444450));
        g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
        g.drawText("B-ZIDE LIMITER", 0, getHeight() - 22, getWidth(), 16, juce::Justification::centred);
    }

    void resized() override
    {
        // Left column: ON/OFF button
        limiterBtn.setBounds(15, 25, 60, 32);

        // Center: Ceiling fader
        ceilingFader.setBounds(120, 22, 40, 160);

        // Mode buttons (right column)
        int modeX = 280;
        int modeY = 116;
        for (int i = 0; i < 3; ++i)
            modeBtns[i].setBounds(modeX + i * 44, modeY, 40, 22);
    }

private:
    BZideProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::TextButton limiterBtn;
    juce::Slider ceilingFader;
    juce::TextButton modeBtns[3];

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterBtnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ceilingAtt;

    float smoothedGR = 0.0f;

    void timerCallback() override
    {
        float rawGR = processor.gainReduction.load();
        // Exponential smoothing: fast attack, slow release
        if (rawGR < smoothedGR)
            smoothedGR += (rawGR - smoothedGR) * 0.4f;  // attack
        else
            smoothedGR += (rawGR - smoothedGR) * 0.08f;  // release

        repaint();
    }

    void paintGRMeter(juce::Graphics& g) const
    {
        // 8 LED dots: 0, -3, -6, -9, -12, -18, -24, -36
        static constexpr float grLevels[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f, -18.0f, -24.0f, -36.0f };
        static constexpr int numLEDs = 8;

        float meterX = 210.0f;
        float meterTop = 24.0f;
        float meterH = 156.0f;
        float ledDiam = 8.0f;
        float spacing = meterH / (float)(numLEDs - 1);

        // Recessed slot
        auto slot = juce::Rectangle<float>(meterX - 4.0f, meterTop - 4.0f, ledDiam + 8.0f, meterH + 8.0f);
        g.setColour(juce::Colour(0xFF0A0A0E));
        g.fillRoundedRectangle(slot, 3.0f);
        g.setColour(juce::Colour(0xFF151518));
        g.drawRoundedRectangle(slot, 3.0f, 0.5f);

        // dB labels
        g.setFont(juce::Font(juce::FontOptions(7.5f)));

        for (int i = 0; i < numLEDs; ++i)
        {
            float cy = meterTop + (float)i * spacing;
            float db = grLevels[i];

            // LED is lit if gain reduction exceeds this level (GR is negative)
            bool lit = smoothedGR <= db && smoothedGR < -0.1f && db != 0.0f;
            if (db == 0.0f)
                lit = smoothedGR < -0.1f; // top LED lights when any GR

            juce::Colour ledCol;
            if (lit)
            {
                if (db >= -3.0f)
                    ledCol = juce::Colour(0xFFDD2200); // red for top
                else if (db >= -9.0f)
                    ledCol = juce::Colour(0xFFf59e0b); // amber
                else
                    ledCol = juce::Colour(0xFFf59e0b).darker(0.3f); // dark amber
            }
            else
            {
                ledCol = juce::Colour(0xFF1A1A1E);
            }

            if (lit)
            {
                // Glow
                g.setColour(ledCol.withAlpha(0.15f));
                g.fillEllipse(meterX - 2.0f, cy - 2.0f, ledDiam + 4.0f, ledDiam + 4.0f);
            }

            g.setColour(ledCol);
            g.fillEllipse(meterX, cy, ledDiam, ledDiam);

            if (lit)
            {
                // Specular highlight
                g.setColour(ledCol.brighter(0.5f).withAlpha(0.5f));
                g.fillEllipse(meterX + 2.0f, cy + 1.5f, 3.0f, 3.0f);
            }

            // dB label to the right
            g.setColour(juce::Colour(0xFF666666));
            juce::String label = (db == 0.0f) ? "0" : juce::String((int)db);
            g.drawText(label, (int)(meterX + ledDiam + 4.0f), (int)(cy - 4.0f), 24, 10,
                       juce::Justification::centredLeft);
        }
    }

    void paintFaderScale(juce::Graphics& g) const
    {
        // Scale marks to the left of the ceiling fader
        float faderLeft = (float)ceilingFader.getX();
        int faderTop = ceilingFader.getY();
        int faderH = ceilingFader.getHeight();

        g.setFont(juce::Font(juce::FontOptions(7.5f)));
        g.setColour(juce::Colour(0xFF666666));

        float dbMarks[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f };
        float minDb = -12.0f;
        float maxDb = 0.0f;

        for (float db : dbMarks)
        {
            float norm = (db - minDb) / (maxDb - minDb);
            float yPos = (float)faderTop + (float)faderH * (1.0f - norm);

            // Tick
            g.setColour(juce::Colour(0xFF444448));
            g.drawHorizontalLine((int)yPos, faderLeft - 6.0f, faderLeft - 1.0f);

            // Label
            g.setColour(juce::Colour(0xFF666666));
            juce::String label = (db == 0.0f) ? "0" : juce::String((int)db);
            g.drawText(label, (int)(faderLeft - 30.0f), (int)(yPos - 4.0f), 22, 9,
                       juce::Justification::centredRight);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterPanel)
};

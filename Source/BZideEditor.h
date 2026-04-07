#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "BZideProcessor.h"

//==============================================================================
// BZideLookAndFeel — Dark Scheps/KI-2A hybrid theme
//==============================================================================
class BZideLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BZideLookAndFeel()
    {
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF0A0A0E));
        setColour(juce::Slider::trackColourId, juce::Colour(0xFF44BB44));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xFFDDDDDD));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF44BB44));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF2A2A30));
        setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
        // KI-2A exact button colors
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1E));   // BTN_BG
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF8B1515)); // BTN_ACTIVE
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF666670));  // TXT_DIM
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1A1A1E));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        float radius = (float)juce::jmin(w, h) * 0.4f;
        float cx = (float)x + (float)w * 0.5f;
        float cy = (float)y + (float)h * 0.5f;
        float angle = startAngle + sliderPos * (endAngle - startAngle);

        // KI-2A style: solid black knob, no border, subtle shadow
        // Shadow
        g.setColour(juce::Colour(0x40000000));
        g.fillEllipse(cx - radius + 1.0f, cy - radius + 1.0f, radius * 2.0f, radius * 2.0f);

        // Knob body — pure black like KI-2A
        g.setColour(juce::Colour(0xFF111114));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // Subtle highlight on top edge (3D effect)
        g.setColour(juce::Colour(0x15FFFFFF));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.5f);

        // White pointer line — KI-2A style (thin, from center to edge)
        float pointerLen = radius * 0.8f;
        float px = cx + std::sin(angle) * pointerLen;
        float py = cy - std::cos(angle) * pointerLen;
        g.setColour(juce::Colour(0xFFEEEEEE));
        g.drawLine(cx + std::sin(angle) * 4.0f, cy - std::cos(angle) * 4.0f, px, py, 2.0f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float /*minPos*/, float /*maxPos*/,
                          juce::Slider::SliderStyle style, juce::Slider&) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // Track
            float trackX = (float)x + (float)w * 0.5f - 3.0f;
            g.setColour(juce::Colour(0xFF1A1A1E));
            g.fillRoundedRectangle(trackX, (float)y, 6.0f, (float)h, 3.0f);

            // dB marks
            g.setColour(juce::Colour(0xFF333333));
            float marks[] = { 0.0f, 0.17f, 0.33f, 0.5f, 0.67f, 0.83f, 1.0f };
            for (float m : marks)
            {
                float my = (float)y + (1.0f - m) * (float)h;
                g.drawHorizontalLine((int)my, trackX - 4.0f, trackX + 10.0f);
            }

            // Fill below thumb
            float fillTop = sliderPos;
            float fillBottom = (float)(y + h);
            g.setColour(juce::Colour(0xFF44BB44).withAlpha(0.3f));
            g.fillRoundedRectangle(trackX, fillTop, 6.0f, fillBottom - fillTop, 3.0f);

            // Fader cap (Pro Tools style)
            float thumbW = (float)w * 0.8f;
            float thumbH = 14.0f;
            float thumbX = (float)x + ((float)w - thumbW) * 0.5f;
            float thumbY = sliderPos - thumbH * 0.5f;

            // Shadow
            g.setColour(juce::Colour(0x40000000));
            g.fillRoundedRectangle(thumbX + 1.0f, thumbY + 1.0f, thumbW, thumbH, 3.0f);

            // Cap body
            juce::ColourGradient capGrad(juce::Colour(0xFF555555), thumbX, thumbY,
                                          juce::Colour(0xFF333333), thumbX, thumbY + thumbH, false);
            g.setGradientFill(capGrad);
            g.fillRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 3.0f);

            // Cap line
            g.setColour(juce::Colour(0xFFDD2200));
            g.drawHorizontalLine((int)(thumbY + thumbH * 0.5f), thumbX + 3.0f, thumbX + thumbW - 3.0f);

            // Cap border
            g.setColour(juce::Colour(0xFF666666));
            g.drawRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 3.0f, 0.5f);
        }
    }
};

//==============================================================================
// BZideEditor — Scheps Omni Channel style with real JUCE components
//==============================================================================
class BZideEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    BZideEditor(BZideProcessor& p);
    ~BZideEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    BZideProcessor& processor;
    BZideLookAndFeel lnf;

    // Layout constants
    static constexpr int kSectionWidth = 120;
    static constexpr int kOutputWidth = 190;
    static constexpr int kHeaderHeight = 28;
    static constexpr int kTotalHeight = 620;

    // Colors
    static constexpr juce::uint32 bgColor        = 0xFF08080A;
    static constexpr juce::uint32 sectionColor    = 0xFF0E0E12;
    static constexpr juce::uint32 headerColor     = 0xFF18181C;
    static constexpr juce::uint32 separatorColor  = 0xFF222228;
    static constexpr juce::uint32 dimTextColor    = 0xFF666666;
    static constexpr juce::uint32 accentColor     = 0xFFDD2200;
    static constexpr juce::uint32 greenColor      = 0xFF44BB44;

    // ── REAL JUCE COMPONENTS ──

    // PRE section
    juce::TextButton preBypass { "PRE" };
    juce::ComboBox preType;
    juce::Slider preDrive, preTone;
    juce::Label preDriveLabel, preToneLabel;

    // EQ section
    juce::TextButton eqBypass { "EQ" };
    juce::Slider eqHpf, eqLpf;
    juce::Slider eqLowFreq, eqLowGain;
    juce::Slider eqMidFreq, eqMidGain, eqMidQ;
    juce::Slider eqHighFreq, eqHighGain;

    // DS² section
    juce::TextButton dsBypass { "DS\xC2\xB2" };
    juce::Slider dsFreq1, dsThresh1, dsFreq2, dsThresh2;

    // COMP section
    juce::TextButton compBypass { "COMP" };
    juce::ComboBox compType;
    juce::Slider compThresh, compRatio, compAttack, compRelease, compMakeup, compMix;

    // GATE section
    juce::TextButton gateBypass { "GATE" };
    juce::ComboBox gateType;
    juce::Slider gateThresh, gateAtten, gateFloor, gateAttack, gateRelease;

    // OUTPUT section
    juce::Slider outFader;
    juce::TextButton limiterBtn { "LIMIT" };
    juce::Slider limiterThresh;
    juce::ComboBox outMode;

    // APVTS attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> preDriveAtt, preToneAtt;
    std::unique_ptr<ComboAttachment> preTypeAtt;
    std::unique_ptr<ButtonAttachment> preBypassAtt;

    std::unique_ptr<ButtonAttachment> eqBypassAtt;
    std::unique_ptr<SliderAttachment> eqHpfAtt, eqLpfAtt;
    std::unique_ptr<SliderAttachment> eqLowFreqAtt, eqLowGainAtt;
    std::unique_ptr<SliderAttachment> eqMidFreqAtt, eqMidGainAtt, eqMidQAtt;
    std::unique_ptr<SliderAttachment> eqHighFreqAtt, eqHighGainAtt;

    std::unique_ptr<ButtonAttachment> dsBypassAtt;
    std::unique_ptr<SliderAttachment> dsFreq1Att, dsThresh1Att, dsFreq2Att, dsThresh2Att;

    std::unique_ptr<ButtonAttachment> compBypassAtt;
    std::unique_ptr<ComboAttachment> compTypeAtt;
    std::unique_ptr<SliderAttachment> compThreshAtt, compRatioAtt, compAttackAtt, compReleaseAtt, compMakeupAtt, compMixAtt;

    std::unique_ptr<ButtonAttachment> gateBypassAtt;
    std::unique_ptr<ComboAttachment> gateTypeAtt;
    std::unique_ptr<SliderAttachment> gateThreshAtt, gateAttenAtt, gateFloorAtt, gateAttackAtt, gateReleaseAtt;

    std::unique_ptr<SliderAttachment> outFaderAtt, limiterThreshAtt;
    std::unique_ptr<ButtonAttachment> limiterBtnAtt;
    std::unique_ptr<ComboAttachment> outModeAtt;

    // Helpers
    void setupRotaryKnob(juce::Slider& s, const juce::String& suffix = "");
    void setupLabel(juce::Label& l, const juce::String& text);
    void drawSectionHeader(juce::Graphics& g, int x, const juce::String& title);
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float levelDb);
    void drawGRMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float grDb);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BZideEditor)
};

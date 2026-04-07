#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "BZideProcessor.h"

//==============================================================================
// BZideEditor — Scheps Omni Channel style vertical strip layout
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

    // Section widths
    static constexpr int kSectionWidth = 110;
    static constexpr int kOutputWidth = 180;
    static constexpr int kHeaderHeight = 28;
    static constexpr int kTotalWidth = kSectionWidth * 6 + kOutputWidth; // 840
    static constexpr int kTotalHeight = 600;

    // Colors (dark theme like Scheps)
    struct Colors {
        static constexpr juce::uint32 bg         = 0xFF1a1a1a;
        static constexpr juce::uint32 sectionBg  = 0xFF222222;
        static constexpr juce::uint32 headerBg   = 0xFF2a2a2a;
        static constexpr juce::uint32 headerText = 0xFFcccccc;
        static constexpr juce::uint32 knobFg     = 0xFFe0e0e0;
        static constexpr juce::uint32 accent     = 0xFFf59e0b; // amber/gold
        static constexpr juce::uint32 activeBtn  = 0xFF4CAF50;
        static constexpr juce::uint32 bypassBtn  = 0xFF666666;
        static constexpr juce::uint32 meterGreen = 0xFF22c55e;
        static constexpr juce::uint32 meterRed   = 0xFFef4444;
        static constexpr juce::uint32 dimText    = 0xFF888888;
        static constexpr juce::uint32 separator  = 0xFF333333;
    };

    // Helper to draw a section
    void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                     const juce::String& title, bool bypassed);
    void drawKnob(juce::Graphics& g, int cx, int cy, int radius,
                  float value, float min, float max, const juce::String& label,
                  const juce::String& unit = "");
    void drawButton(juce::Graphics& g, juce::Rectangle<int> bounds,
                    const juce::String& text, bool active);
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds,
                   float levelDb, float minDb = -60.0f, float maxDb = 6.0f);
    void drawGRMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float grDb);

    // Trial banner
    void drawTrialBanner(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BZideEditor)
};

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "BZideProcessor.h"
#include "BinaryData.h"

//==============================================================================
// BZideLookAndFeel - Dark Scheps/KI-2A hybrid theme
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
        float radius = (float)juce::jmin(w, h) * 0.32f;
        float cx = (float)x + (float)w * 0.5f;
        float cy = (float)y + (float)h * 0.5f;
        float angle = startAngle + sliderPos * (endAngle - startAngle);
        float pi = juce::MathConstants<float>::pi;
        float twoPi = juce::MathConstants<float>::twoPi;

        // ── 1. Drop shadow ──
        g.setColour(juce::Colour(0x40000000));
        g.fillEllipse(cx - radius - 4.0f, cy - radius + 2.0f, (radius + 4.0f) * 2.0f, (radius + 6.0f) * 2.0f);

        // ── 2. Outer ring — dark with knurling grip ──
        float outerR = radius + 2.0f;
        {
            juce::ColourGradient outerGrad(
                juce::Colour(0xFF333338), cx, cy - outerR,
                juce::Colour(0xFF151518), cx, cy + outerR, false);
            g.setGradientFill(outerGrad);
            g.fillEllipse(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f);

            // Knurling — fine grip lines
            int numKnurl = 48;
            for (int i = 0; i < numKnurl; ++i)
            {
                float ka = (float)i / (float)numKnurl * twoPi;
                float kr1 = outerR - 1.5f;
                float kr2 = outerR;
                float kx1 = cx + std::sin(ka) * kr1;
                float ky1 = cy - std::cos(ka) * kr1;
                float kx2 = cx + std::sin(ka) * kr2;
                float ky2 = cy - std::cos(ka) * kr2;
                g.setColour((i % 2 == 0) ? juce::Colour(0x18FFFFFF) : juce::Colour(0x10000000));
                g.drawLine(kx1, ky1, kx2, ky2, 0.5f);
            }
        }

        // ── 3. Groove between outer and middle ring ──
        float grooveR = radius - 1.0f;
        g.setColour(juce::Colour(0xFF060608));
        g.drawEllipse(cx - grooveR - 0.5f, cy - grooveR - 0.5f, (grooveR + 0.5f) * 2.0f, (grooveR + 0.5f) * 2.0f, 1.5f);

        // ── 4. Middle ring — diamond/faceted chrome effect ──
        float midOuterR = grooveR - 1.0f;
        float midInnerR = radius * 0.75f;
        {
            // Base chrome fill
            juce::ColourGradient chromeGrad(
                juce::Colour(0xFF666670), cx, cy - midOuterR,
                juce::Colour(0xFF2A2A30), cx, cy + midOuterR, false);
            g.setGradientFill(chromeGrad);
            g.fillEllipse(cx - midOuterR, cy - midOuterR, midOuterR * 2.0f, midOuterR * 2.0f);

            // Cut out the inner circle
            g.setColour(juce::Colour(0xFF0E0E12));
            g.fillEllipse(cx - midInnerR, cy - midInnerR, midInnerR * 2.0f, midInnerR * 2.0f);

            // Faceted star reflections — alternating bright/dark segments
            int numFacets = 10;
            g.saveState();
            {
                // Clip to the ring area (donut shape)
                juce::Path ringClip;
                ringClip.addEllipse(cx - midOuterR, cy - midOuterR, midOuterR * 2.0f, midOuterR * 2.0f);
                juce::Path innerHole;
                innerHole.addEllipse(cx - midInnerR, cy - midInnerR, midInnerR * 2.0f, midInnerR * 2.0f);
                ringClip.setUsingNonZeroWinding(false);
                ringClip.addPath(innerHole);
                g.reduceClipRegion(ringClip);

                for (int i = 0; i < numFacets; ++i)
                {
                    float a1 = (float)i / (float)numFacets * twoPi;
                    float a2 = (float)(i + 1) / (float)numFacets * twoPi;
                    float aMid = (a1 + a2) * 0.5f;

                    juce::Path facet;
                    facet.startNewSubPath(cx, cy);
                    facet.lineTo(cx + std::sin(a1) * midOuterR * 1.2f, cy - std::cos(a1) * midOuterR * 1.2f);
                    facet.lineTo(cx + std::sin(a2) * midOuterR * 1.2f, cy - std::cos(a2) * midOuterR * 1.2f);
                    facet.closeSubPath();

                    // Alternate bright and dark facets
                    float brightness = (i % 2 == 0) ? 0.18f : 0.06f;
                    // Add variation based on angle relative to "light source" (top-left)
                    float lightAngle = -pi * 0.3f;
                    float facetAngle = aMid - lightAngle;
                    float lightFactor = (1.0f + std::cos(facetAngle)) * 0.5f;
                    brightness *= (0.5f + lightFactor);

                    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(brightness));
                    g.fillPath(facet);
                }
            }
            g.restoreState();

            // Rim highlights on the middle ring
            g.setColour(juce::Colour(0x15FFFFFF));
            g.drawEllipse(cx - midOuterR + 0.5f, cy - midOuterR + 0.5f,
                          (midOuterR - 0.5f) * 2.0f, (midOuterR - 0.5f) * 2.0f, 0.5f);
        }

        // ── 5. Inner groove ──
        g.setColour(juce::Colour(0xFF050508));
        g.drawEllipse(cx - midInnerR - 0.5f, cy - midInnerR - 0.5f,
                      (midInnerR + 0.5f) * 2.0f, (midInnerR + 0.5f) * 2.0f, 1.0f);

        // ── 6. Inner cap — dark center with subtle dome ──
        float capR = midInnerR - 1.0f;
        {
            juce::ColourGradient capGrad(
                juce::Colour(0xFF1A1A20), cx - capR * 0.2f, cy - capR * 0.3f,
                juce::Colour(0xFF08080C), cx, cy + capR, true);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

            // Subtle specular
            g.setColour(juce::Colour(0x10FFFFFF));
            g.fillEllipse(cx - capR * 0.4f, cy - capR * 0.5f, capR * 0.6f, capR * 0.4f);
        }

        // ── 7. LED ring channel ──
        {
            float ringR = outerR + 3.0f;
            float ringW = 4.0f;

            g.setColour(juce::Colour(0xFF050508));
            g.drawEllipse(cx - ringR - ringW * 0.5f, cy - ringR - ringW * 0.5f,
                          (ringR + ringW * 0.5f) * 2.0f, (ringR + ringW * 0.5f) * 2.0f, 0.8f);

            juce::Path ringPath;
            ringPath.addEllipse(cx - ringR - ringW * 0.5f, cy - ringR - ringW * 0.5f,
                                (ringR + ringW * 0.5f) * 2.0f, (ringR + ringW * 0.5f) * 2.0f);
            juce::Path innerCut;
            innerCut.addEllipse(cx - ringR + ringW * 0.5f, cy - ringR + ringW * 0.5f,
                                (ringR - ringW * 0.5f) * 2.0f, (ringR - ringW * 0.5f) * 2.0f);
            ringPath.setUsingNonZeroWinding(false);
            ringPath.addPath(innerCut);
            g.setColour(juce::Colour(0xFF0A0A0D));
            g.fillPath(ringPath);
        }

        // ── 8. LED dots ──
        int numDots = 17;
        float dotRadius = outerR + 3.0f;
        float dotSize = 1.5f;
        for (int i = 0; i < numDots; ++i)
        {
            float t = (float)i / (float)(numDots - 1);
            float dotAngle = startAngle + t * (endAngle - startAngle);
            float dx = cx + std::sin(dotAngle) * dotRadius;
            float dy = cy - std::cos(dotAngle) * dotRadius;
            bool lit = (dotAngle <= angle && sliderPos > 0.01f);

            if (lit)
            {
                g.setColour(juce::Colour(0xFFDD2200).withAlpha(0.2f));
                g.fillEllipse(dx - dotSize * 2.0f, dy - dotSize * 2.0f, dotSize * 4.0f, dotSize * 4.0f);
                g.setColour(juce::Colour(0xFFDD2200));
                g.fillEllipse(dx - dotSize, dy - dotSize, dotSize * 2.0f, dotSize * 2.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xFF2A2A30));
                g.fillEllipse(dx - dotSize * 0.7f, dy - dotSize * 0.7f, dotSize * 1.4f, dotSize * 1.4f);
            }
        }

        // ── 9. Pointer / indicator ──
        {
            float pointerLen = midOuterR - 2.0f;
            float pointerStart = capR * 0.3f;
            float sx = cx + std::sin(angle) * pointerStart;
            float sy = cy - std::cos(angle) * pointerStart;
            float px = cx + std::sin(angle) * pointerLen;
            float py = cy - std::cos(angle) * pointerLen;

            g.setColour(juce::Colour(0x25FFFFFF));
            g.drawLine(sx, sy, px, py, 3.0f);
            g.setColour(juce::Colour(0xFFEEEEEE));
            g.drawLine(sx, sy, px, py, 1.5f);
            g.setColour(juce::Colours::white);
            g.fillEllipse(px - 1.2f, py - 1.2f, 2.4f, 2.4f);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float, float,
                          juce::Slider::SliderStyle style, juce::Slider&) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // Track — dark groove, full height
            float trackX = (float)x + (float)w * 0.5f - 3.0f;
            g.setColour(juce::Colour(0xFF0A0A0E));
            g.fillRoundedRectangle(trackX, (float)y, 6.0f, (float)h, 3.0f);

            // Fine center line
            g.setColour(juce::Colour(0xFF333338));
            g.drawLine(trackX + 3.0f, (float)y + 4.0f, trackX + 3.0f, (float)(y + h) - 4.0f, 0.5f);

            // dB marks
            g.setColour(juce::Colour(0xFF333333));
            float marks[] = { 0.0f, 0.17f, 0.33f, 0.5f, 0.67f, 0.83f, 1.0f };
            for (float m : marks)
            {
                float my = (float)y + (1.0f - m) * (float)h;
                g.drawHorizontalLine((int)my, trackX - 4.0f, trackX + 10.0f);
            }

            // Fill below thumb (subtle green)
            float fillTop = sliderPos;
            float fillBottom = (float)(y + h);
            g.setColour(juce::Colour(0xFF44BB44).withAlpha(0.15f));
            g.fillRoundedRectangle(trackX, fillTop, 6.0f, fillBottom - fillTop, 3.0f);

            // ── Fader thumb — clean chrome cap ──
            float thumbW = (float)w * 0.85f;
            float thumbH = 48.0f;
            float thumbX = (float)x + ((float)w - thumbW) * 0.5f;
            float thumbY = sliderPos - thumbH * 0.5f;
            float cr = 2.0f;

            float halfH = (thumbH - 2.0f) * 0.5f;
            float topY = thumbY;
            float botY = thumbY + halfH + 2.0f;

            // Dark frame
            g.setColour(juce::Colour(0xFF111114));
            g.fillRoundedRectangle(thumbX - 0.5f, thumbY - 0.5f, thumbW + 1.0f, thumbH + 1.0f, cr + 0.5f);

            // TOP HALF
            g.setColour(juce::Colour(0xFF888890));
            g.fillRoundedRectangle(thumbX, topY, thumbW, halfH, cr);

            // Top highlight
            g.setColour(juce::Colour(0x30FFFFFF));
            g.drawHorizontalLine((int)(topY + 1), thumbX + cr, thumbX + thumbW - cr);

            // CENTER DIVIDE
            g.setColour(juce::Colour(0xFF0A0A0E));
            g.fillRect(thumbX + 1.0f, topY + halfH, thumbW - 2.0f, 2.0f);

            // BOTTOM HALF
            g.setColour(juce::Colour(0xFF999AA0));
            g.fillRoundedRectangle(thumbX, botY, thumbW, halfH, cr);

            // Bottom edge
            g.setColour(juce::Colour(0x20000000));
            g.drawHorizontalLine((int)(botY + halfH - 1), thumbX + cr, thumbX + thumbW - cr);
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto area = btn.getLocalBounds().toFloat().reduced(1.0f);
        bool active = btn.getToggleState();
        float cornerRadius = 4.0f;

        g.setColour(juce::Colour(0x60000000));
        g.fillRoundedRectangle(area.translated(0.0f, 2.0f), cornerRadius);
        g.setColour(juce::Colour(0x30000000));
        g.fillRoundedRectangle(area.translated(0.0f, 1.0f), cornerRadius);

        if (active)
        {
            g.setColour(juce::Colour(0xFF8B1515).withAlpha(0.15f));
            g.fillRoundedRectangle(area.expanded(5.0f), 6.0f);
            g.setColour(juce::Colour(0xFF8B1515).withAlpha(0.3f));
            g.fillRoundedRectangle(area.expanded(2.0f), 5.0f);

            juce::ColourGradient activeGrad(
                juce::Colour(0xFFA52020), area.getX(), area.getY(),
                juce::Colour(0xFF6B0E0E), area.getX(), area.getBottom(), false);
            g.setGradientFill(activeGrad);
            g.fillRoundedRectangle(area, cornerRadius);

            g.setColour(juce::Colour(0x30FFFFFF));
            g.drawHorizontalLine((int)area.getY(), area.getX() + 2.0f, area.getRight() - 2.0f);
            g.setColour(juce::Colour(0x40000000));
            g.drawHorizontalLine((int)area.getBottom() - 1, area.getX() + 2.0f, area.getRight() - 2.0f);
            g.setColour(juce::Colour(0xFFAA2222).withAlpha(0.5f));
            g.drawRoundedRectangle(area, cornerRadius, 0.8f);

            if (isMouseOver)
            {
                g.setColour(juce::Colour(0x18FFFFFF));
                g.fillRoundedRectangle(area, cornerRadius);
            }
        }
        else
        {
            juce::ColourGradient offGrad(
                juce::Colour(0xFF252528), area.getX(), area.getY(),
                juce::Colour(0xFF111114), area.getX(), area.getBottom(), false);
            g.setGradientFill(offGrad);
            g.fillRoundedRectangle(area, cornerRadius);

            g.setColour(juce::Colour(0x20FFFFFF));
            g.drawHorizontalLine((int)area.getY(), area.getX() + 2.0f, area.getRight() - 2.0f);
            g.setColour(juce::Colour(0x40000000));
            g.drawHorizontalLine((int)area.getBottom() - 1, area.getX() + 2.0f, area.getRight() - 2.0f);
            g.setColour(juce::Colour(0xFF555558));
            g.drawRoundedRectangle(area, cornerRadius, 0.8f);

            if (isMouseOver)
            {
                g.setColour(juce::Colour(0x12FFFFFF));
                g.fillRoundedRectangle(area, cornerRadius);
            }
        }

        if (isButtonDown)
        {
            g.setColour(juce::Colour(0x35000000));
            g.fillRoundedRectangle(area, cornerRadius);
            g.setColour(juce::Colour(0x30000000));
            g.drawHorizontalLine((int)area.getY() + 1, area.getX() + 2.0f, area.getRight() - 2.0f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool, bool) override
    {
        auto area = btn.getLocalBounds();
        bool active = btn.getToggleState();
        g.setColour(active ? juce::Colours::white : juce::Colour(0xFF666670));
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f).withStyle("Bold")));
        g.drawText(btn.getButtonText(), area, juce::Justification::centred, false);
    }
};

//==============================================================================
// LED Component - KI-2A style bombilla with glow
//==============================================================================
class LEDComponent : public juce::Component
{
public:
    LEDComponent() { setInterceptsMouseClicks(false, false); setOpaque(false); }

    void setOn(bool on) { if (isOn != on) { isOn = on; repaint(); } }
    void setColor(juce::Colour c) { color = c; }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float r = 4.0f;

        if (isOn)
        {
            g.setColour(color.withAlpha(0.2f));
            g.fillEllipse(b);
            g.setColour(color.withAlpha(0.4f));
            g.fillEllipse(cx - r * 2.0f, cy - r * 2.0f, r * 4.0f, r * 4.0f);
            g.setColour(color);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.fillEllipse(cx - 1.5f, cy - 1.5f, 3.0f, 3.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xFF333338));
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }
    }

private:
    bool isOn = false;
    juce::Colour color { 0xFFDD2200 };
};

// Forward declarations for section types
#include "UI/ChannelSection.h"
#include "UI/PreSection.h"
#include "UI/EQSection.h"
#include "UI/DS2Section.h"
#include "UI/CompSection.h"
#include "UI/GateSection.h"
#include "UI/InsertSection.h"
#include "UI/OutputSection.h"

//==============================================================================
// BZideEditor - Section-based architecture with drag-to-reorder
//==============================================================================
class BZideEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    BZideEditor(BZideProcessor& p);
    ~BZideEditor() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    BZideProcessor& processor;
    BZideLookAndFeel lnf;

    // Layout constants
    static constexpr int kSectionWidth = 140;
    static constexpr int kOutputWidth = 340;
    static constexpr int kTotalHeight = 800;

    // Section components
    std::unique_ptr<PreSection> preSection;
    std::unique_ptr<EQSection> eqSection;
    std::unique_ptr<DS2Section> ds2Section;
    std::unique_ptr<CompSection> compSection;
    std::unique_ptr<GateSection> gateSection;
    std::unique_ptr<InsertSection> insertSection;
    std::unique_ptr<OutputSection> outputSection;

    // Draggable order (indices into SectionId: PRE=0, EQ=1, DS2=2, COMP=3, GATE=4)
    std::array<SectionId, 5> draggableOrder = { SectionId::PRE, SectionId::EQ, SectionId::DS2, SectionId::COMP, SectionId::GATE };

    // Get section pointer by SectionId
    ChannelSection* getSectionById(SectionId id);

    // Drag handling
    void handleDragMove(ChannelSection* section, int deltaX);
    void handleDragEnd(ChannelSection* section);

    // Find index in draggableOrder
    int findDragIndex(SectionId id);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BZideEditor)
};

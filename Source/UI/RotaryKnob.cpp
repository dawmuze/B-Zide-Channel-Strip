#include "RotaryKnob.h"
#include <cmath>

static const juce::Colour ACCENT     (0xFFDD2200);
static const juce::Colour ACCENT_DIM (0xFF331100);
static const juce::Colour KNOB_DARK  (0xFF141418);
static const juce::Colour KNOB_RING  (0xFF222228);
static const juce::Colour KNOB_EDGE  (0xFF3A3A40);

// Smooth gradient: dark gray → light gray → white
static juce::Colour getLedColor (float t)
{
    // t = 0.0 → dim gray, 1.0 → bright white
    float brightness = 0.35f + t * 0.65f;
    return juce::Colour::fromFloatRGBA (brightness, brightness, brightness * 0.97f, 1.f);
}

RotaryKnob::RotaryKnob() {}

void RotaryKnob::setValue (float v)
{
    value = juce::jlimit (minVal, maxVal, v);
    repaint();
}

void RotaryKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (6.f);
    float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto centre = bounds.getCentre();
    float outerR = size * 0.42f;
    float innerR = size * 0.30f;
    float arcR = size * 0.38f;

    // Normalized value 0..1
    float normalized = (value - minVal) / (maxVal - minVal);

    // Arc angles: 7 o'clock (-225°) to 5 o'clock (+45°) = 270° sweep
    // In JUCE radians: start at 3/4 pi going clockwise
    float startAngle = juce::MathConstants<float>::pi * 0.75f;   // 135° = 7 o'clock
    float endAngle   = juce::MathConstants<float>::pi * 2.25f;   // 405° = 5 o'clock
    float valueAngle = startAngle + normalized * (endAngle - startAngle);

    // ── Full dark circle behind entire knob area ──
    {
        float bgR = size * 0.50f;
        g.setColour (juce::Colour (0xF00A0A0E));
        g.fillEllipse (centre.x - bgR, centre.y - bgR, bgR * 2.f, bgR * 2.f);
    }

    // ── LED ring (inside the dark circle, with depth) ──
    {
        int numLEDs = 21;
        float ledR = size * 0.015f;
        float ledDist = outerR * 1.08f;  // just outside knob but inside dark bg

        for (int i = 0; i < numLEDs; i++)
        {
            float ledNorm = static_cast<float> (i) / (numLEDs - 1);
            float ledAngle = startAngle + ledNorm * (endAngle - startAngle);

            float lx = centre.x + ledDist * std::cos (ledAngle);
            float ly = centre.y + ledDist * std::sin (ledAngle);

            bool lit = ledNorm <= normalized;
            juce::Colour ledCol = getLedColor (ledNorm);

            // Recessed hole (dark shadow for depth)
            g.setColour (juce::Colour (0xFF020204));
            g.fillEllipse (lx - ledR * 1.4f, ly - ledR * 1.4f, ledR * 2.8f, ledR * 2.8f);

            // Inner shadow (top-left darker = recessed look)
            g.setColour (juce::Colour (0xFF060608));
            g.fillEllipse (lx - ledR * 1.1f, ly - ledR * 1.1f, ledR * 2.2f, ledR * 2.2f);

            if (lit)
            {
                // Glow from inside the hole
                g.setColour (ledCol.withAlpha (0.2f));
                g.fillEllipse (lx - ledR * 1.8f, ly - ledR * 1.8f, ledR * 3.6f, ledR * 3.6f);

                // Lit LED
                g.setColour (ledCol);
                g.fillEllipse (lx - ledR * 0.8f, ly - ledR * 0.8f, ledR * 1.6f, ledR * 1.6f);

                // Bright specular center
                g.setColour (juce::Colours::white.withAlpha (0.3f));
                g.fillEllipse (lx - ledR * 0.3f, ly - ledR * 0.3f, ledR * 0.6f, ledR * 0.6f);
            }
            else
            {
                // Unlit: just the dark hole
                g.setColour (juce::Colour (0xFF101014));
                g.fillEllipse (lx - ledR * 0.8f, ly - ledR * 0.8f, ledR * 1.6f, ledR * 1.6f);
            }
        }
    }

    // ── Background arc track (dim) ──
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f,
                             startAngle, endAngle, true);
        g.setColour (ACCENT_DIM);
        g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // ── Active arc (red, up to current value) ──
    if (normalized > 0.01f)
    {
        juce::Path active;
        active.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f,
                              startAngle, valueAngle, true);
        g.setColour (ACCENT);
        g.strokePath (active, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        // Glow
        g.setColour (ACCENT.withAlpha (0.15f));
        g.strokePath (active, juce::PathStrokeType (7.f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // ── Outer knob body (dark metallic) ──
    {
        juce::ColourGradient ringGrad (KNOB_EDGE, centre.x, centre.y - outerR,
                                        KNOB_DARK, centre.x, centre.y + outerR, false);
        g.setGradientFill (ringGrad);
        g.fillEllipse (centre.x - outerR, centre.y - outerR, outerR * 2.f, outerR * 2.f);
    }

    // ── Inner knob ──
    {
        juce::ColourGradient bodyGrad (juce::Colour (0xFF2A2A30), centre.x - innerR * 0.3f, centre.y - innerR * 0.3f,
                                        KNOB_DARK, centre.x + innerR, centre.y + innerR, true);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (centre.x - innerR, centre.y - innerR, innerR * 2.f, innerR * 2.f);

        g.setColour (juce::Colour (0xFF3A3A42));
        g.drawEllipse (centre.x - innerR, centre.y - innerR, innerR * 2.f, innerR * 2.f, 0.6f);
    }

    // ── Value indicator line ──
    {
        float lineInner = innerR * 0.1f;
        float lineOuter = innerR * 0.80f;

        float x1 = centre.x + lineInner * std::cos (valueAngle);
        float y1 = centre.y + lineInner * std::sin (valueAngle);
        float x2 = centre.x + lineOuter * std::cos (valueAngle);
        float y2 = centre.y + lineOuter * std::sin (valueAngle);

        // Glow
        g.setColour (juce::Colour (0x30AAAAAA));
        g.drawLine (x1, y1, x2, y2, 4.f);

        // Line
        g.setColour (juce::Colour (0xFFBBBBBB));
        g.drawLine (x1, y1, x2, y2, 2.f);
    }
}

void RotaryKnob::mouseDown (const juce::MouseEvent& e)
{
    dragStartY = e.y;
    dragStartValue = value;
}

void RotaryKnob::mouseDrag (const juce::MouseEvent& e)
{
    float sensitivity = 0.15f;
    float delta = static_cast<float> (dragStartY - e.y) * sensitivity;
    setValue (dragStartValue + delta);
    if (onValueChange)
        onValueChange (value);
}

void RotaryKnob::mouseDoubleClick (const juce::MouseEvent&)
{
    setValue (0.f);
    if (onValueChange)
        onValueChange (value);
}

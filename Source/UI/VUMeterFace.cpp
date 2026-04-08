#include "VUMeterFace.h"
#include "BinaryData.h"
#include <cmath>

VUMeterFace::VUMeterFace()
{
    lastTimeMs = juce::Time::getMillisecondCounterHiRes();
    loadFaceImage();
}

void VUMeterFace::loadFaceImage()
{
    faceImage = juce::ImageCache::getFromMemory (BinaryData::meter_face_gold_png,
                                                  BinaryData::meter_face_gold_pngSize);
}

void VUMeterFace::setLevel (float dB)
{
    double now = juce::Time::getMillisecondCounterHiRes();
    double elapsed = now - lastTimeMs;
    lastTimeMs = now;
    if (elapsed > 100.0) elapsed = 16.67;

    ballistics.process (dB, elapsed);
    repaint();
}

void VUMeterFace::setMeterMode (MeterMode mode)
{
    ballistics.setMode (mode);
    repaint();
}

void VUMeterFace::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    if (faceImage.isValid())
    {
        g.drawImage (faceImage, bounds,
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

    float scaleX = bounds.getWidth() / style::imgW;
    float scaleY = bounds.getHeight() / style::imgH;
    float scale = juce::jmin (scaleX, scaleY);

    float offsetX = (bounds.getWidth() - style::imgW * scale) * 0.5f + bounds.getX();
    float offsetY = (bounds.getHeight() - style::imgH * scale) * 0.5f + bounds.getY();

    float pivotX = offsetX + style::imgPivotX * scale;
    float pivotY = offsetY + style::imgPivotY * scale;
    float radius = style::imgRadius * scale;

    juce::Point<float> pivot (pivotX, pivotY);

    float angleDeg = style::dbToAngleDeg (ballistics.getCurrentDb());
    drawNeedle (g, pivot, radius, angleDeg);
}

void VUMeterFace::drawNeedle (juce::Graphics& g, juce::Point<float> pivot, float radius, float angleDeg)
{
    float angleRad = juce::degreesToRadians (angleDeg - 90.f);
    float needleLen = radius * 0.98f;

    float cosA = std::cos (angleRad);
    float sinA = std::sin (angleRad);

    float tipX = pivot.x + needleLen * cosA;
    float tipY = pivot.y + needleLen * sinA;

    float perpX = sinA;
    float perpY = -cosA;

    // Shadow
    {
        juce::Path shadow;
        float bw = 2.f, tw = 0.2f, sx = 1.5f, sy = 2.f;
        shadow.startNewSubPath (pivot.x + sx - perpX * bw, pivot.y + sy - perpY * bw);
        shadow.lineTo (tipX + sx - perpX * tw, tipY + sy - perpY * tw);
        shadow.lineTo (tipX + sx + perpX * tw, tipY + sy + perpY * tw);
        shadow.lineTo (pivot.x + sx + perpX * bw, pivot.y + sy + perpY * bw);
        shadow.closeSubPath();
        g.setColour (juce::Colour (0x28000000));
        g.fillPath (shadow);
    }

    // Needle
    {
        juce::Path needle;
        float bw = 1.5f, tw = 0.15f;
        needle.startNewSubPath (pivot.x - perpX * bw, pivot.y - perpY * bw);
        needle.lineTo (tipX - perpX * tw, tipY - perpY * tw);
        needle.lineTo (tipX + perpX * tw, tipY + perpY * tw);
        needle.lineTo (pivot.x + perpX * bw, pivot.y + perpY * bw);
        needle.closeSubPath();
        g.setColour (juce::Colour (0xFF111111));
        g.fillPath (needle);
    }

    // Pivot hub
    float hubR = juce::jmax (4.f, radius * 0.018f);
    g.setColour (juce::Colour (0xFF1A1816));
    g.fillEllipse (pivot.x - hubR, pivot.y - hubR, hubR * 2.f, hubR * 2.f);
    g.setColour (juce::Colour (0xFF2D2A26));
    g.fillEllipse (pivot.x - hubR * 0.6f, pivot.y - hubR * 0.6f, hubR * 1.2f, hubR * 1.2f);
}

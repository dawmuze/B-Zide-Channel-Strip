#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace style
{

// Image coordinate system for meter face (958x524 gold Dawmuze)
constexpr float imgW = 958.f;
constexpr float imgH = 524.f;
constexpr float imgPivotX = 479.f;
constexpr float imgPivotY = 470.f;   // lower pivot = wider needle sweep
constexpr float imgRadius = 260.f;

// dB-to-angle mapping
struct ScalePoint { float dB; float angleDeg; };
// Measured from the Dawmuze gold VU meter image pixels
// Angles calculated from pivot (479,430) to each scale mark
constexpr ScalePoint scaleMap[] = {
    { -20.f, -64.f },
    { -10.f, -48.f },
    {  -7.f, -43.f },
    {  -5.f, -29.f },
    {  -3.f, -15.f },
    {  -2.f,  -9.f },
    {  -1.f,  -2.f },
    {   0.f,  23.f },
    {   1.f,  24.f },
    {   2.f,  42.f },
    {   3.f,  70.f },
};
constexpr int scaleMapSize = 11;

inline float dbToAngleDeg (float dB)
{
    if (dB <= scaleMap[0].dB) return scaleMap[0].angleDeg;
    if (dB >= scaleMap[scaleMapSize - 1].dB) return scaleMap[scaleMapSize - 1].angleDeg;

    for (int i = 0; i < scaleMapSize - 1; ++i)
    {
        if (dB >= scaleMap[i].dB && dB <= scaleMap[i + 1].dB)
        {
            float t = (dB - scaleMap[i].dB) / (scaleMap[i + 1].dB - scaleMap[i].dB);
            return scaleMap[i].angleDeg + t * (scaleMap[i + 1].angleDeg - scaleMap[i].angleDeg);
        }
    }
    return 0.f;
}

// ── Theme system for standalone UI components ──
struct Theme
{
    juce::Colour toolbarBg     { 0xFF18181C };
    juce::Colour toolbarText   { 0xFFAAAAAA };
    juce::Colour toolbarActive { 0xFFDD2200 };
    juce::Colour readoutGreen  { 0xFF44BB44 };
};

inline Theme getTheme (int /*index*/) { return Theme {}; }
inline Theme getWavesTheme()          { return Theme {}; }

} // namespace style

#pragma once
#include "ChannelSection.h"

class DS2Section : public ChannelSection
{
public:
    DS2Section(juce::AudioProcessorValueTreeState& apvts)
        : ChannelSection(SectionId::DS2, "DE-ESSER", 140, true)
    {
        dsBypassAtt = std::make_unique<BA>(apvts, "ds_bypass", bypassBtn);

        // Gray tones for LED dots — no purple
        setupKnob(dsFreq1);
        dsFreq1Att = std::make_unique<SA>(apvts, "ds_freq1", dsFreq1);
        dsFreq1.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFBBBBBB));

        setupKnob(dsThresh1);
        dsThresh1Att = std::make_unique<SA>(apvts, "ds_thresh1", dsThresh1);
        dsThresh1.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF888888));

        setupKnob(dsFreq2);
        dsFreq2Att = std::make_unique<SA>(apvts, "ds_freq2", dsFreq2);
        dsFreq2.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFAAAAAA));

        setupKnob(dsThresh2);
        dsThresh2Att = std::make_unique<SA>(apvts, "ds_thresh2", dsThresh2);
        dsThresh2.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF777777));

        // Output knob (display-only)
        setupKnob(dsOutput, " dB");
        dsOutputAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ds_output", dsOutput);
        dsOutput.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF999999));

        // SC button (sidechain)
        scBtn.setClickingTogglesState(true);
        addAndMakeVisible(scBtn);

        // Listen buttons (solo the de-esser band)
        listenBtn1.setButtonText("");
        listenBtn1.setClickingTogglesState(true);
        addAndMakeVisible(listenBtn1);

        listenBtn2.setButtonText("");
        listenBtn2.setClickingTogglesState(true);
        addAndMakeVisible(listenBtn2);
    }

protected:
    void paintSectionContent(juce::Graphics& g) override
    {
        // --- Recessed header drawing lambda ---
        auto drawSectionHeader = [&](int headerY, const juce::String& text) {
            int hdrH = 18;
            auto hdrArea = juce::Rectangle<int>(6, headerY, sectionWidth - 12, hdrH);
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(hdrArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(hdrArea.reduced(1).toFloat(), 2.0f);
            g.setColour(juce::Colour(0x25000000));
            g.drawHorizontalLine(hdrArea.getY() + 1, (float)hdrArea.getX() + 2, (float)hdrArea.getRight() - 2);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(hdrArea.getBottom() - 2, (float)hdrArea.getX() + 2, (float)hdrArea.getRight() - 2);
            float cy = (float)(headerY + hdrH / 2);
            g.setColour(juce::Colour(0xFF444448));
            int textHalfW = (int)(text.length() * 3.5f) + 4;
            g.drawHorizontalLine((int)cy, (float)hdrArea.getX() + 4, (float)(sectionWidth / 2 - textHalfW));
            g.drawHorizontalLine((int)cy, (float)(sectionWidth / 2 + textHalfW), (float)hdrArea.getRight() - 4);
            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText(text, hdrArea, juce::Justification::centred);
        };

        // --- GR LED dots drawing lambda ---
        auto drawGRDots = [&](float dotX, float startY, float spacing) {
            for (int d = 0; d < 4; ++d)
            {
                float dotY = startY + (float)d * spacing;
                // Static: all off for now (no GR data yet)
                g.setColour(juce::Colour(0xFF2A2A30));
                g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
                // Subtle ring
                g.setColour(juce::Colour(0xFF06b6d4).withAlpha(0.15f));
                g.drawEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f, 0.5f);
            }
            // Labels beside dots
            g.setColour(juce::Colour(0xFF555558));
            g.setFont(juce::Font(juce::FontOptions(7.0f)));
            const char* labels[] = { "-3", "-6", "-9", "-12" };
            for (int d = 0; d < 4; ++d)
            {
                float dotY = startY + (float)d * spacing;
                g.drawText(labels[d], (int)(dotX + 5), (int)(dotY - 5), 20, 10, juce::Justification::centredLeft);
            }
        };

        // --- Divider line drawing lambda ---
        auto drawDivider = [&](int divY) {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawHorizontalLine(divY, 8.0f, (float)(sectionWidth - 8));
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(divY + 1, 8.0f, (float)(sectionWidth - 8));
        };

        // --- Draw BAND 1 ---
        drawSectionHeader(band1HeaderY, "BAND 1");
        drawDivider(band1HeaderY + 20); // subtle divider below BAND 1 header
        if (thresh1HeaderY > 0)
            drawSectionHeader(thresh1HeaderY, "THRESHOLD");

        // GR dots to the left of thresh knob
        float grDotX1 = 18.0f;
        float grStartY1 = (float)dsThresh1.getY() + 6.0f;
        drawGRDots(grDotX1, grStartY1, 10.0f);

        drawDivider(band1DividerY);

        // --- Draw BAND 2 ---
        drawSectionHeader(band2HeaderY, "BAND 2");
        drawDivider(band2HeaderY + 20); // subtle divider below BAND 2 header
        if (thresh2HeaderY > 0)
            drawSectionHeader(thresh2HeaderY, "THRESHOLD");

        // GR dots to the left of thresh knob
        float grDotX2 = 18.0f;
        float grStartY2 = (float)dsThresh2.getY() + 6.0f;
        drawGRDots(grDotX2, grStartY2, 10.0f);

        drawDivider(band2DividerY);

        // --- Draw OUTPUT ---
        drawSectionHeader(outputHeaderY, "OUTPUT");

        // --- Draw SIDECHAIN ---
        if (scHeaderY > 0)
        {
            drawDivider(scHeaderY - 4);
            drawSectionHeader(scHeaderY, "SIDECHAIN");
        }
    }

    void paintSectionLabels(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xFF666666));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());

        // Simple labels only
        g.setColour(juce::Colour(0xFF999999));
        g.setFont(juce::Font(juce::FontOptions(8.0f)).boldened());
        labelAbove(g, dsFreq1, "FREQ");
        labelAbove(g, dsFreq2, "FREQ");
        labelAbove(g, dsOutput, "GAIN");

        // Speaker icons on listen buttons (painted over children)
        auto drawSpeaker = [&](const juce::TextButton& btn) {
            auto b = btn.getBounds().toFloat().reduced(3.0f);
            float cx = b.getCentreX();
            float cy = b.getCentreY();
            bool on = btn.getToggleState();
            juce::Colour col = on ? juce::Colour(0xFFf97316) : juce::Colour(0xFF888888);

            // Speaker body (small rectangle)
            g.setColour(col);
            g.fillRect(cx - 3.0f, cy - 2.5f, 4.0f, 5.0f);

            // Speaker cone (triangle)
            juce::Path cone;
            cone.startNewSubPath(cx + 1.0f, cy - 2.5f);
            cone.lineTo(cx + 5.0f, cy - 5.0f);
            cone.lineTo(cx + 5.0f, cy + 5.0f);
            cone.lineTo(cx + 1.0f, cy + 2.5f);
            cone.closeSubPath();
            g.fillPath(cone);

            // Sound waves when ON
            if (on)
            {
                g.setColour(col.withAlpha(0.7f));
                juce::Path wave1;
                wave1.addCentredArc(cx + 6.0f, cy, 3.0f, 3.0f, 0.0f, -0.6f, 0.6f, true);
                g.strokePath(wave1, juce::PathStrokeType(1.0f));
            }
        };

        drawSpeaker(listenBtn1);
        drawSpeaker(listenBtn2);
    }

    void resizeSectionContent() override
    {
        int y = getContentStartY();
        int freqKnob = 100;   // FREQ = big knob (same as saturator)
        int threshKnob = 44;  // THRESHOLD = small knob
        int outputKnob = 100;  // OUTPUT = big knob (same as FREQ)

        // ====== BAND 1 ======
        band1HeaderY = y + 4;
        y += 30;

        // FREQ knob (big)
        y += 10;
        centerKnob(dsFreq1, y, freqKnob);
        y += freqKnob + 24;  // space for value text below knob

        // THRESHOLD header + knob (small)
        thresh1HeaderY = y;
        y += 24;
        centerKnob(dsThresh1, y, threshKnob);
        listenBtn1.setBounds(sectionWidth - 26, y + threshKnob / 2 - 10, 20, 20);
        y += threshKnob + 20;  // space for value text below knob

        band1DividerY = y;
        y += 8;

        // ====== BAND 2 ======
        band2HeaderY = y;
        y += 30;

        // FREQ knob (big)
        y += 10;
        centerKnob(dsFreq2, y, freqKnob);
        y += freqKnob + 24;  // space for value text below knob

        // THRESHOLD header + knob (small)
        thresh2HeaderY = y;
        y += 24;
        centerKnob(dsThresh2, y, threshKnob);
        listenBtn2.setBounds(sectionWidth - 26, y + threshKnob / 2 - 10, 20, 20);
        y += threshKnob + 20;  // space for value text below knob

        band2DividerY = y;
        y += 8;

        // ====== OUTPUT ======
        outputHeaderY = y;
        y += 26;
        y += 6;
        centerKnob(dsOutput, y, outputKnob);
        y += outputKnob + 20;

        // SIDECHAIN header + SC button
        scHeaderY = y;
        y += 22;
        int scW = sectionWidth - 20;
        scBtn.setBounds((sectionWidth - scW) / 2, y, scW, 22);
    }

private:
    juce::Slider dsFreq1, dsThresh1, dsFreq2, dsThresh2;
    juce::Slider dsOutput; // display-only output knob
    juce::TextButton listenBtn1, listenBtn2;
    juce::TextButton scBtn { "SC" };

    // Stored Y positions for paint
    int band1HeaderY = 0, band1DividerY = 0;
    int band2HeaderY = 0, band2DividerY = 0;
    int outputHeaderY = 0;
    int thresh1HeaderY = 0;
    int thresh2HeaderY = 0;
    int scHeaderY = 0;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BA> dsBypassAtt;
    std::unique_ptr<SA> dsFreq1Att, dsThresh1Att, dsFreq2Att, dsThresh2Att, dsOutputAtt;
};

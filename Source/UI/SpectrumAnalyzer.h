#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../BZideProcessor.h"

class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzer(BZideProcessor& proc)
        : processor(proc),
          forwardFFT(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        setSize(600, 300);
        std::fill(smoothedMagnitudes.begin(), smoothedMagnitudes.end(), -100.0f);
        startTimerHz(30);
    }

    ~SpectrumAnalyzer() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float w = bounds.getWidth();
        float h = bounds.getHeight();

        // Dark gradient background
        juce::ColourGradient bgGrad(juce::Colour(0xFF0A0A10), 0, 0,
                                     juce::Colour(0xFF14141A), 0, h, false);
        g.setGradientFill(bgGrad);
        g.fillRect(bounds);

        // Draw grid
        drawGrid(g, w, h);

        // Draw spectrum
        drawSpectrum(g, w, h);

        // Draw EQ curve
        drawEQCurve(g, w, h);

        // Draw draggable nodes
        drawNodes(g, w, h);

        // Border
        g.setColour(juce::Colour(0xFF2A2A30));
        g.drawRect(bounds, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        float w = (float)getWidth();
        float h = (float)getHeight();

        // Check if clicking on a node
        draggedNode = -1;
        for (int i = 0; i < 3; ++i)
        {
            float nx = getNodeX(i, w);
            float ny = getNodeY(i, h);
            float dist = std::sqrt((e.x - nx) * (e.x - nx) + (e.y - ny) * (e.y - ny));
            if (dist < 12.0f)
            {
                draggedNode = i;
                break;
            }
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggedNode < 0) return;

        float w = (float)getWidth();
        float h = (float)getHeight();

        float freq = xToFreq(juce::jlimit(0.0f, w, (float)e.x), w);
        float gain = yToDb(juce::jlimit(0.0f, h, (float)e.y), h);

        auto& apvts = processor.getAPVTS();

        switch (draggedNode)
        {
            case 0: // HIGH
            {
                freq = juce::jlimit(2000.0f, 20000.0f, freq);
                gain = juce::jlimit(-18.0f, 18.0f, gain);
                if (auto* p = apvts.getParameter("eq_high_freq"))
                    p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(freq));
                if (auto* p = apvts.getParameter("eq_high_gain"))
                    p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(gain));
                break;
            }
            case 1: // MID
            {
                freq = juce::jlimit(200.0f, 8000.0f, freq);
                gain = juce::jlimit(-18.0f, 18.0f, gain);
                if (auto* p = apvts.getParameter("eq_mid_freq"))
                    p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(freq));
                if (auto* p = apvts.getParameter("eq_mid_gain"))
                    p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(gain));
                break;
            }
            case 2: // LOW
            {
                freq = juce::jlimit(30.0f, 500.0f, freq);
                gain = juce::jlimit(-18.0f, 18.0f, gain);
                if (auto* p = apvts.getParameter("eq_low_freq"))
                    p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(freq));
                if (auto* p = apvts.getParameter("eq_low_gain"))
                    p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(gain));
                break;
            }
        }

        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggedNode = -1;
    }

private:
    BZideProcessor& processor;

    // FFT
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder; // 2048
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, fftSize * 2> fftDataLocal {};
    std::array<float, fftSize / 2> smoothedMagnitudes {};

    // Dragging
    int draggedNode = -1; // 0=HIGH, 1=MID, 2=LOW

    // Coordinate mapping
    float freqToX(float freq, float width) const
    {
        float minLog = std::log10(20.0f);
        float maxLog = std::log10(20000.0f);
        return (std::log10(freq) - minLog) / (maxLog - minLog) * width;
    }

    float xToFreq(float x, float width) const
    {
        float minLog = std::log10(20.0f);
        float maxLog = std::log10(20000.0f);
        float logFreq = minLog + (x / width) * (maxLog - minLog);
        return std::pow(10.0f, logFreq);
    }

    float dbToY(float db, float height) const
    {
        float minDb = -24.0f, maxDb = 18.0f;
        return height * (1.0f - (db - minDb) / (maxDb - minDb));
    }

    float yToDb(float y, float height) const
    {
        float minDb = -24.0f, maxDb = 18.0f;
        return minDb + (1.0f - y / height) * (maxDb - minDb);
    }

    // Node positions
    float getNodeX(int nodeIndex, float width) const
    {
        auto& apvts = processor.getAPVTS();
        float freq = 1000.0f;
        switch (nodeIndex)
        {
            case 0: freq = *apvts.getRawParameterValue("eq_high_freq"); break;
            case 1: freq = *apvts.getRawParameterValue("eq_mid_freq"); break;
            case 2: freq = *apvts.getRawParameterValue("eq_low_freq"); break;
        }
        return freqToX(freq, width);
    }

    float getNodeY(int nodeIndex, float height) const
    {
        auto& apvts = processor.getAPVTS();
        float gain = 0.0f;
        switch (nodeIndex)
        {
            case 0: gain = *apvts.getRawParameterValue("eq_high_gain"); break;
            case 1: gain = *apvts.getRawParameterValue("eq_mid_gain"); break;
            case 2: gain = *apvts.getRawParameterValue("eq_low_gain"); break;
        }
        return dbToY(gain, height);
    }

    juce::Colour getNodeColour(int nodeIndex) const
    {
        switch (nodeIndex)
        {
            case 0: return juce::Colour(0xFFf97316); // HIGH = orange
            case 1: return juce::Colour(0xFFeab308); // MID = yellow
            case 2: return juce::Colour(0xFF22c55e); // LOW = green
            default: return juce::Colours::white;
        }
    }

    juce::String getNodeLabel(int nodeIndex) const
    {
        switch (nodeIndex)
        {
            case 0: return "H";
            case 1: return "M";
            case 2: return "L";
            default: return "";
        }
    }

    // Drawing helpers
    void drawGrid(juce::Graphics& g, float w, float h)
    {
        g.setColour(juce::Colour(0x15FFFFFF));

        // Frequency grid lines
        float freqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float f : freqs)
        {
            float x = freqToX(f, w);
            g.drawVerticalLine((int)x, 0, h);
        }

        // dB grid lines
        float dbs[] = { -24, -18, -12, -6, 0, 6, 12, 18 };
        for (float db : dbs)
        {
            float y = dbToY(db, h);
            if (db == 0.0f)
            {
                g.setColour(juce::Colour(0x25FFFFFF));
                g.drawHorizontalLine((int)y, 0, w);
                g.setColour(juce::Colour(0x15FFFFFF));
            }
            else
            {
                g.drawHorizontalLine((int)y, 0, w);
            }
        }

        // Frequency labels
        g.setColour(juce::Colour(0xFF555560));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        const char* freqLabels[] = { "20", "50", "100", "200", "500", "1K", "2K", "5K", "10K", "20K" };
        for (int i = 0; i < 10; ++i)
        {
            float x = freqToX(freqs[i], w);
            g.drawText(freqLabels[i], (int)(x - 15), (int)(h - 14), 30, 12, juce::Justification::centred);
        }

        // dB labels
        for (float db : dbs)
        {
            float y = dbToY(db, h);
            juce::String label = (db > 0 ? "+" : "") + juce::String((int)db);
            g.drawText(label, 2, (int)(y - 6), 28, 12, juce::Justification::centredLeft);
        }
    }

    void drawSpectrum(juce::Graphics& g, float w, float h)
    {
        juce::Path spectrumPath;
        bool started = false;

        double sampleRate = processor.getSampleRate();
        if (sampleRate <= 0) sampleRate = 44100.0;

        for (int i = 0; i < (int)w; ++i)
        {
            float freq = xToFreq((float)i, w);
            if (freq < 20.0f || freq > 20000.0f) continue;

            // Find the corresponding FFT bin
            int bin = (int)(freq / sampleRate * (float)fftSize);
            if (bin < 0 || bin >= fftSize / 2) continue;

            float mag = smoothedMagnitudes[(size_t)bin];

            // Clamp to display range
            mag = juce::jlimit(-100.0f, 18.0f, mag);
            float y = dbToY(mag, h);
            y = juce::jlimit(0.0f, h, y);

            if (!started)
            {
                spectrumPath.startNewSubPath((float)i, y);
                started = true;
            }
            else
            {
                spectrumPath.lineTo((float)i, y);
            }
        }

        if (!spectrumPath.isEmpty())
        {
            // Create filled version
            juce::Path filledPath = spectrumPath;
            filledPath.lineTo(w, h);
            filledPath.lineTo(0, h);
            filledPath.closeSubPath();

            // Gradient fill
            juce::ColourGradient fillGrad(
                juce::Colour(0x4006b6d4), 0, 0,
                juce::Colour(0x0006b6d4), 0, h, false);
            g.setGradientFill(fillGrad);
            g.fillPath(filledPath);

            // Spectrum line
            g.setColour(juce::Colour(0xAA06b6d4));
            g.strokePath(spectrumPath, juce::PathStrokeType(1.0f));
        }
    }

    void drawEQCurve(juce::Graphics& g, float w, float h)
    {
        auto& apvts = processor.getAPVTS();
        double sampleRate = processor.getSampleRate();
        if (sampleRate <= 0) sampleRate = 44100.0;

        float hpf = *apvts.getRawParameterValue("eq_hpf");
        float lpf = *apvts.getRawParameterValue("eq_lpf");
        float lowF = *apvts.getRawParameterValue("eq_low_freq");
        float lowG = *apvts.getRawParameterValue("eq_low_gain");
        float midF = *apvts.getRawParameterValue("eq_mid_freq");
        float midG = *apvts.getRawParameterValue("eq_mid_gain");
        float midQ = *apvts.getRawParameterValue("eq_mid_q");
        float hiF = *apvts.getRawParameterValue("eq_high_freq");
        float hiG = *apvts.getRawParameterValue("eq_high_gain");

        // Get filter coefficients
        auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpf);
        auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowF, 0.707f,
            juce::Decibels::decibelsToGain(lowG));
        auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, midF, midQ,
            juce::Decibels::decibelsToGain(midG));
        auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, hiF, 0.707f,
            juce::Decibels::decibelsToGain(hiG));
        auto lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpf);

        juce::Path eqPath;
        bool started = false;

        for (int i = 0; i < (int)w; ++i)
        {
            float freq = xToFreq((float)i, w);
            if (freq < 20.0f || freq > 20000.0f) continue;

            float totalMag = 1.0f;
            totalMag *= getMagnitudeForCoeffs(*hpfCoeffs, freq, sampleRate);
            totalMag *= getMagnitudeForCoeffs(*lowCoeffs, freq, sampleRate);
            totalMag *= getMagnitudeForCoeffs(*midCoeffs, freq, sampleRate);
            totalMag *= getMagnitudeForCoeffs(*highCoeffs, freq, sampleRate);
            totalMag *= getMagnitudeForCoeffs(*lpfCoeffs, freq, sampleRate);

            float db = juce::Decibels::gainToDecibels(totalMag);
            db = juce::jlimit(-24.0f, 18.0f, db);
            float y = dbToY(db, h);

            if (!started)
            {
                eqPath.startNewSubPath((float)i, y);
                started = true;
            }
            else
            {
                eqPath.lineTo((float)i, y);
            }
        }

        if (!eqPath.isEmpty())
        {
            // Glow
            g.setColour(juce::Colour(0x30f97316));
            g.strokePath(eqPath, juce::PathStrokeType(4.0f));

            // Main line
            g.setColour(juce::Colour(0xFFf97316));
            g.strokePath(eqPath, juce::PathStrokeType(2.0f));
        }
    }

    float getMagnitudeForCoeffs(const juce::dsp::IIR::Coefficients<float>& coeffs,
                                float freq, double sampleRate) const
    {
        auto* c = coeffs.getRawCoefficients();
        // Biquad: H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)
        // where a0 is normalized to 1
        float b0 = c[0], b1 = c[1], b2 = c[2];
        float a0 = c[3], a1 = c[4], a2 = c[5];

        // JUCE stores coefficients as [b0, b1, b2, a0, a1, a2]
        // but IIR::Coefficients normalizes a0 = 1.0
        // Actually JUCE stores [b0, b1, b2, a0, a1, a2] where a0 is already 1
        float omega = (float)(2.0 * juce::MathConstants<double>::pi * freq / sampleRate);
        float cosW = std::cos(omega);
        float sinW = std::sin(omega);
        float cos2W = std::cos(2.0f * omega);
        float sin2W = std::sin(2.0f * omega);

        // Numerator: b0 + b1*e^(-jw) + b2*e^(-2jw)
        float numReal = b0 + b1 * cosW + b2 * cos2W;
        float numImag = -(b1 * sinW + b2 * sin2W);

        // Denominator: a0 + a1*e^(-jw) + a2*e^(-2jw)
        float denReal = a0 + a1 * cosW + a2 * cos2W;
        float denImag = -(a1 * sinW + a2 * sin2W);

        float numMag = std::sqrt(numReal * numReal + numImag * numImag);
        float denMag = std::sqrt(denReal * denReal + denImag * denImag);

        return denMag > 0.0f ? numMag / denMag : 1.0f;
    }

    void drawNodes(juce::Graphics& g, float w, float h)
    {
        for (int i = 0; i < 3; ++i)
        {
            float nx = getNodeX(i, w);
            float ny = getNodeY(i, h);
            auto colour = getNodeColour(i);
            float radius = (draggedNode == i) ? 10.0f : 8.0f;

            // Glow
            g.setColour(colour.withAlpha(0.2f));
            g.fillEllipse(nx - radius * 2.0f, ny - radius * 2.0f, radius * 4.0f, radius * 4.0f);

            // Outer ring
            g.setColour(colour.withAlpha(0.6f));
            g.fillEllipse(nx - radius, ny - radius, radius * 2.0f, radius * 2.0f);

            // Inner fill
            g.setColour(colour);
            g.fillEllipse(nx - radius + 2.0f, ny - radius + 2.0f,
                         (radius - 2.0f) * 2.0f, (radius - 2.0f) * 2.0f);

            // Label
            g.setColour(juce::Colour(0xFF000000));
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText(getNodeLabel(i), (int)(nx - 6), (int)(ny - 6), 12, 12,
                       juce::Justification::centred);

            // Tooltip with freq/gain values
            auto& apvts = processor.getAPVTS();
            float freq = 0, gain = 0;
            switch (i)
            {
                case 0: freq = *apvts.getRawParameterValue("eq_high_freq");
                        gain = *apvts.getRawParameterValue("eq_high_gain"); break;
                case 1: freq = *apvts.getRawParameterValue("eq_mid_freq");
                        gain = *apvts.getRawParameterValue("eq_mid_gain"); break;
                case 2: freq = *apvts.getRawParameterValue("eq_low_freq");
                        gain = *apvts.getRawParameterValue("eq_low_gain"); break;
            }

            juce::String tooltip;
            if (freq >= 1000.0f)
                tooltip = juce::String(freq / 1000.0f, 1) + "kHz";
            else
                tooltip = juce::String((int)freq) + "Hz";
            tooltip += "  " + juce::String(gain, 1) + "dB";

            g.setColour(juce::Colour(0xBBFFFFFF));
            g.setFont(juce::Font(juce::FontOptions(9.0f)));
            float labelY = (ny > h * 0.5f) ? ny - radius - 14.0f : ny + radius + 2.0f;
            g.drawText(tooltip, (int)(nx - 35), (int)labelY, 70, 12, juce::Justification::centred);
        }
    }

    void timerCallback() override
    {
        if (processor.nextFFTBlockReady)
        {
            // Copy data
            std::copy(processor.fftData, processor.fftData + fftSize * 2, fftDataLocal.begin());
            processor.nextFFTBlockReady = false;

            // Apply window
            window.multiplyWithWindowingTable(fftDataLocal.data(), fftSize);

            // Perform FFT
            forwardFFT.performFrequencyOnlyForwardTransform(fftDataLocal.data());

            // Convert to dB and smooth
            double sampleRate = processor.getSampleRate();
            if (sampleRate <= 0) sampleRate = 44100.0;

            for (int i = 0; i < fftSize / 2; ++i)
            {
                float mag = fftDataLocal[(size_t)i];
                float db = juce::Decibels::gainToDecibels(mag / (float)fftSize, -100.0f);

                // Exponential smoothing
                float smoothing = 0.7f;
                smoothedMagnitudes[(size_t)i] = smoothedMagnitudes[(size_t)i] * smoothing + db * (1.0f - smoothing);
            }

            repaint();
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};

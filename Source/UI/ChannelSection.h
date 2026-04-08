#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "BinaryData.h"

enum class SectionId { PRE = 0, EQ = 1, DS2 = 2, COMP = 3, GATE = 4, INSERT = 5, OUTPUT = 6 };

class ChannelSection : public juce::Component
{
public:
    ChannelSection(SectionId id, const juce::String& sectionTitle, int width, bool draggable)
        : sectionId(id), title(sectionTitle), sectionWidth(width), canDrag(draggable)
    {
        setPaintingIsUnclipped(true); // allow children to paint outside section bounds
        if (draggable)
        {
            // ON button in the header
            bypassBtn.setClickingTogglesState(true);
            addAndMakeVisible(bypassBtn);

            // ST / DUO / MS buttons
            for (auto* b : { &stBtn, &duoBtn, &msBtn })
            {
                b->setClickingTogglesState(true);
                b->setRadioGroupId(9000 + (int)id);
                addAndMakeVisible(b);
            }
            stBtn.setToggleState(true, juce::dontSendNotification);
        }
    }

    virtual ~ChannelSection() override = default;

    SectionId getSectionId() const { return sectionId; }
    int getSectionWidth() const { return sectionWidth; }
    bool isDraggable() const { return canDrag; }
    void setDragHighlight(bool h) { dragHighlight = h; repaint(); }
    juce::TextButton& getBypassButton() { return bypassBtn; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        int shadowGap = 3;

        // Shadow gaps on left/right (dark near-black strips)
        g.setColour(juce::Colour(0xFF020204));
        g.fillRect(0, 0, shadowGap, bounds.getHeight());
        g.fillRect(bounds.getWidth() - shadowGap, 0, shadowGap, bounds.getHeight());

        // Content area (between shadow gaps)
        auto content = bounds.reduced(shadowGap, 0);

        // Leather texture background
        auto bgImage = juce::ImageCache::getFromMemory(BinaryData::back_jpg, BinaryData::back_jpgSize);
        if (bgImage.isValid())
        {
            g.saveState();
            g.reduceClipRegion(content);
            g.drawImage(bgImage, content.toFloat(), juce::RectanglePlacement::stretchToFit);
            g.restoreState();
        }

        // Dark overlay
        g.setColour(juce::Colour(0xFF0E0E12).withAlpha(0.85f));
        g.fillRect(content);

        // Bevel edges - raised panel effect
        // Left bright edge
        g.setColour(juce::Colour(0x20FFFFFF));
        g.drawVerticalLine(shadowGap, 0.0f, (float)bounds.getHeight());
        // Right dark edge
        g.setColour(juce::Colour(0x30000000));
        g.drawVerticalLine(bounds.getWidth() - shadowGap - 1, 0.0f, (float)bounds.getHeight());

        // Header bar — deep 3D inset panel
        auto headerArea = content.removeFromTop(kHeaderHeight);
        {
            // Outer shadow (top dark inset)
            g.setColour(juce::Colour(0xFF060608));
            g.fillRect(headerArea);

            // Inner raised area
            auto inner = headerArea.reduced(1, 1);
            juce::ColourGradient headerGrad(
                juce::Colour(0xFF2A2A35), (float)inner.getX(), (float)inner.getY(),
                juce::Colour(0xFF101018), (float)inner.getX(), (float)inner.getBottom(), false);
            g.setGradientFill(headerGrad);
            g.fillRect(inner);

            // Top highlight — light catches the top lip
            g.setColour(juce::Colour(0x28FFFFFF));
            g.drawHorizontalLine(inner.getY(), (float)inner.getX() + 2.0f, (float)inner.getRight() - 2.0f);

            // Bottom shadow
            g.setColour(juce::Colour(0x40000000));
            g.drawHorizontalLine(inner.getBottom() - 1, (float)inner.getX() + 2.0f, (float)inner.getRight() - 2.0f);

            // Left bright edge
            g.setColour(juce::Colour(0x12FFFFFF));
            g.drawVerticalLine(inner.getX(), (float)inner.getY() + 1, (float)inner.getBottom() - 1);

            // Right dark edge
            g.setColour(juce::Colour(0x18000000));
            g.drawVerticalLine(inner.getRight() - 1, (float)inner.getY() + 1, (float)inner.getBottom() - 1);
        }

        // Header title — centered, bright orange
        g.setColour(juce::Colour(0xFFf97316));
        g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
        g.drawText(title, headerArea, juce::Justification::centred);

        // Drag grip icon (3 horizontal lines) for draggable sections
        if (canDrag)
        {
            float gx = (float)(headerArea.getRight() - 16);
            float gy = (float)(headerArea.getCentreY() - 5);
            g.setColour(juce::Colour(0x40FFFFFF));
            for (int i = 0; i < 3; ++i)
                g.fillRect(gx, gy + (float)i * 4.0f, 8.0f, 1.5f);
        }

        // Drop shadow below header
        for (int i = 0; i < 4; ++i)
        {
            float alpha = 0.15f * (1.0f - (float)i / 4.0f);
            g.setColour(juce::Colour(0xFF000000).withAlpha(alpha));
            g.drawHorizontalLine(headerArea.getBottom() + i, (float)(bounds.getX() + shadowGap), (float)(bounds.getRight() - shadowGap));
        }

        // Drag highlight
        if (dragHighlight)
        {
            g.setColour(juce::Colour(0xFFDD2200).withAlpha(0.3f));
            g.drawRect(bounds.reduced(shadowGap, 0), 2);
        }

        // Divider line below ST/DUO/MS buttons
        if (canDrag)
        {
            int divY = kHeaderHeight + kStereoBtnH + 3;
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawHorizontalLine(divY, (float)(shadowGap + 4), (float)(bounds.getWidth() - shadowGap - 4));
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(divY + 1, (float)(shadowGap + 4), (float)(bounds.getWidth() - shadowGap - 4));
        }

        // Let subclass paint its content
        paintSectionContent(g);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        paintSectionLabels(g);
    }

    void resized() override
    {
        if (canDrag)
        {
            // ON button in the header — left side
            bypassBtn.setBounds(6, 4, 32, kHeaderHeight - 8);

            // ST/DUO/MS buttons right below header
            int btnW = (sectionWidth - 12) / 3;
            int bx = 4;
            int by = kHeaderHeight + 2;
            stBtn.setBounds(bx, by, btnW, kStereoBtnH);
            duoBtn.setBounds(bx + btnW + 2, by, btnW, kStereoBtnH);
            msBtn.setBounds(bx + (btnW + 2) * 2, by, btnW, kStereoBtnH);
        }
        resizeSectionContent();
    }

    // Mouse drag on header
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (canDrag && e.y < kHeaderHeight)
        {
            dragging = true;
            dragStartX = e.getScreenX();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragging && onDragMove)
        {
            int deltaX = e.getScreenX() - dragStartX;
            onDragMove(this, deltaX);
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragging)
        {
            dragging = false;
            if (onDragEnd) onDragEnd(this);
        }
    }

    // Reset drag origin after a swap so delta starts fresh
    void resetDragOrigin() { dragStartX = juce::Desktop::getMousePosition().x; }

    // Callbacks set by BZideEditor
    std::function<void(ChannelSection*, int)> onDragMove;
    std::function<void(ChannelSection*)> onDragEnd;

    static constexpr int kHeaderHeight = 28;

protected:
    virtual void paintSectionContent(juce::Graphics&) {}
    virtual void paintSectionLabels(juce::Graphics&) {}
    virtual void resizeSectionContent() {}

    // Helper for subclasses to setup rotary knobs
    void setupKnob(juce::Slider& s, const juce::String& suffix = "")
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 13);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFF999999));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF0A0A0E));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        s.setPaintingIsUnclipped(true); // LED ring can paint outside bounds
        if (suffix.isNotEmpty()) s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
    }

    // Content area below header (and ST/DUO/MS row if draggable)
    juce::Rectangle<int> getContentArea() const
    {
        int topOffset = kHeaderHeight + 4;
        if (canDrag) topOffset += kStereoBtnH + 4; // account for ST/DUO/MS row
        return getLocalBounds().withTrimmedLeft(3).withTrimmedRight(3).withTrimmedTop(topOffset);
    }

    // Y where section content starts (after header + stereo buttons)
    int getContentStartY() const
    {
        return canDrag ? (kHeaderHeight + kStereoBtnH + 6) : (kHeaderHeight + 4);
    }

    // Layout helpers for knobs
    void centerKnob(juce::Slider& s, int sy, int size)
    {
        int pad = 14;
        int w = size + pad;
        int h = size + 22; // extra height so text box has space below knob
        s.setBounds((sectionWidth - w) / 2, sy, w, h);
    }

    void centerKnobPair(juce::Slider& s1, juce::Slider& s2, int sy, int size)
    {
        int pad = 14;
        int gap = 4;
        int w = size + pad;
        int h = size + 22;
        int totalW = w * 2 + gap;
        int startX = (sectionWidth - totalW) / 2;
        s1.setBounds(startX, sy, w, h);
        s2.setBounds(startX + w + gap, sy, w, h);
    }

    // Label helpers for paintSectionLabels
    void labelAbove(juce::Graphics& g, const juce::Slider& s, const juce::String& text)
    {
        g.drawText(text, s.getX() - 10, s.getY() - 14, s.getWidth() + 20, 11, juce::Justification::centred);
    }

    void labelBetween(juce::Graphics& g, const juce::Slider& s1, const juce::Slider& s2,
                      const juce::String& t1, const juce::String& t2)
    {
        g.drawText(t1, s1.getX() - 10, s1.getY() - 14, s1.getWidth() + 20, 11, juce::Justification::centred);
        g.drawText(t2, s2.getX() - 10, s2.getY() - 14, s2.getWidth() + 20, 11, juce::Justification::centred);
    }

    SectionId sectionId;
    juce::String title;
    int sectionWidth;
    bool canDrag;
    bool dragHighlight = false;
    bool dragging = false;
    int dragStartX = 0;

    // ON bypass button (in header)
    juce::TextButton bypassBtn { "ON" };

    // ST / DUO / MS stereo mode buttons (per-section)
    juce::TextButton stBtn  { "ST" };
    juce::TextButton duoBtn { "DUO" };
    juce::TextButton msBtn  { "MS" };

    static constexpr int kStereoBtnH = 16;
};

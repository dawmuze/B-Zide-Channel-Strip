#pragma once
#include "ChannelSection.h"
#include "../BZideProcessor.h"

class InsertSection : public ChannelSection
{
public:
    InsertSection(BZideProcessor& proc)
        : ChannelSection(SectionId::INSERT, "INSERT", 140, true),
          processor_(proc)
    {
        // Create 10 insert slot buttons (2 cols × 5 rows)
        for (int i = 0; i < numSlots; ++i)
        {
            auto& btn = slotBtns_[i];
            btn.setButtonText("---");
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF222228));
            btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.3f));
            btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white.withAlpha(0.3f));

            btn.onClick = [this, i]() { onSlotClicked(i); };
            btn.addMouseListener(this, false);
            addAndMakeVisible(btn);
        }

        // PRE / POST fader routing buttons
        preBtn_.setClickingTogglesState(true);
        preBtn_.setRadioGroupId(2001);
        preBtn_.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(preBtn_);

        postBtn_.setClickingTogglesState(true);
        postBtn_.setRadioGroupId(2001);
        addAndMakeVisible(postBtn_);

        syncSlotsFromProcessor();
    }

    void syncSlotsFromProcessor()
    {
        for (int i = 0; i < numSlots; ++i)
        {
            auto& slot = processor_.insertSlots_[i];
            auto& btn = slotBtns_[i];

            if (slot.isLoaded())
            {
                btn.setButtonText(InsertProcessor::getModuleName(slot.getModuleType()));
                if (slot.isBypassed())
                {
                    btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E3E2E));
                    btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF886644));
                }
                else
                {
                    btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E3E2E));
                    btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF88CC88));
                }
            }
            else
            {
                btn.setButtonText("---");
                btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF222228));
                btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.3f));
            }
        }
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override
    {
        auto pos = e.getEventRelativeTo(this).getPosition();

        // Right-click on slot buttons → context menu
        if (e.mods.isRightButtonDown())
        {
            for (int i = 0; i < numSlots; ++i)
            {
                if (slotBtns_[i].getBounds().contains(pos) && processor_.insertSlots_[i].isLoaded())
                {
                    showSlotContextMenu(i);
                    return;
                }
            }
        }

        // Left-click on loaded slot → start drag tracking
        if (e.mods.isLeftButtonDown())
        {
            for (int i = 0; i < numSlots; ++i)
            {
                if (slotBtns_[i].getBounds().contains(pos) && processor_.insertSlots_[i].isLoaded())
                {
                    dragSourceSlot_ = i;
                    dragActive_ = false; // not dragging yet, just mouseDown
                    return;
                }
            }
        }

        ChannelSection::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragSourceSlot_ >= 0)
        {
            if (!dragActive_ && e.getDistanceFromDragStart() > 5)
                dragActive_ = true;

            if (dragActive_)
            {
                // Highlight target slot
                auto pos = e.getEventRelativeTo(this).getPosition();
                int newTarget = -1;
                for (int i = 0; i < numSlots; ++i)
                {
                    if (slotBtns_[i].getBounds().contains(pos) && i != dragSourceSlot_)
                        newTarget = i;
                }

                if (newTarget != dragTargetSlot_)
                {
                    dragTargetSlot_ = newTarget;
                    repaint();
                }
                return;
            }
        }
        ChannelSection::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (dragSourceSlot_ >= 0 && dragActive_ && dragTargetSlot_ >= 0)
        {
            // Swap the slots
            processor_.swapInserts(dragSourceSlot_, dragTargetSlot_);
            syncSlotsFromProcessor();
        }
        else if (dragSourceSlot_ >= 0 && !dragActive_)
        {
            // Was a click, not a drag — do normal click action
            onSlotClicked(dragSourceSlot_);
        }

        dragSourceSlot_ = -1;
        dragTargetSlot_ = -1;
        dragActive_ = false;
        repaint();

        ChannelSection::mouseUp(e);
    }

    void paintSectionContent(juce::Graphics& g) override
    {
        float dL = 8.0f;
        float dR = (float)(sectionWidth - 8);

        auto drawDiv = [&](int divY) {
            g.setColour(juce::Colour(0xFF2A2A30));
            g.drawHorizontalLine(divY, dL, dR);
            g.setColour(juce::Colour(0x10FFFFFF));
            g.drawHorizontalLine(divY + 1, dL, dR);
        };

        // "INSERTS" header label — Pro Tools style
        int headerY = getContentStartY();
        {
            auto headerArea = juce::Rectangle<int>(6, headerY, sectionWidth - 12, 16);
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(headerArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(headerArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("INSERTS", headerArea, juce::Justification::centred);
        }

        // Slot number labels (A-J) painted to the left of each row
        if (slotBtns_[0].getHeight() > 0)
        {
            g.setColour(juce::Colour(0xFF444448));
            g.setFont(juce::Font(juce::FontOptions(7.0f)).boldened());

            for (int row = 0; row < 5; ++row)
            {
                int slotIdx = row; // left column
                auto& btn = slotBtns_[slotIdx];
                char label = (char)('A' + row);
                g.drawText(juce::String::charToString(label),
                           1, btn.getY(), 8, btn.getHeight(),
                           juce::Justification::centred);
            }
        }

        // Divider below slots
        if (slotBtns_[0].getHeight() > 0)
        {
            int lastSlotBottom = slotBtns_[4].getBottom(); // bottom of left column
            drawDiv(lastSlotBottom + 4);
        }

        // "ROUTING" header
        if (preBtn_.getHeight() > 0)
        {
            int routY = preBtn_.getY() - 20;
            auto routArea = juce::Rectangle<int>(6, routY, sectionWidth - 12, 16);
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(routArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(routArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("ROUTING", routArea, juce::Justification::centred);
        }

        // "SIGNAL FLOW" label at bottom
        {
            int sigY = preBtn_.getBottom() + 16;
            auto sigArea = juce::Rectangle<int>(6, sigY, sectionWidth - 12, 16);
            g.setColour(juce::Colour(0xFF060608));
            g.fillRoundedRectangle(sigArea.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xFF0C0C10));
            g.fillRoundedRectangle(sigArea.reduced(1).toFloat(), 2.0f);

            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText("SIGNAL FLOW", sigArea, juce::Justification::centred);
        }

        // Signal chain diagram (vertical arrows showing slot order)
        if (slotBtns_[0].getHeight() > 0)
        {
            int diagramY = preBtn_.getBottom() + 36;
            int diagramH = getHeight() - diagramY - 20;
            int cx = sectionWidth / 2;

            g.setColour(juce::Colour(0xFF333338));
            g.drawLine((float)cx, (float)diagramY, (float)cx, (float)(diagramY + diagramH), 1.0f);

            // Arrow head at bottom
            juce::Path arrow;
            arrow.startNewSubPath((float)cx, (float)(diagramY + diagramH));
            arrow.lineTo((float)(cx - 4), (float)(diagramY + diagramH - 8));
            arrow.lineTo((float)(cx + 4), (float)(diagramY + diagramH - 8));
            arrow.closeSubPath();
            g.setColour(juce::Colour(0xFF333338));
            g.fillPath(arrow);

            // Dots for each loaded slot
            g.setFont(juce::Font(juce::FontOptions(7.0f)));
            int dotSpacing = diagramH / (numSlots + 1);
            for (int i = 0; i < numSlots; ++i)
            {
                int dotY = diagramY + (i + 1) * dotSpacing;
                if (processor_.insertSlots_[i].isLoaded())
                {
                    juce::Colour dotCol = processor_.insertSlots_[i].isBypassed()
                        ? juce::Colour(0xFF886644)
                        : juce::Colour(0xFF88CC88);
                    g.setColour(dotCol);
                    g.fillEllipse((float)(cx - 3), (float)(dotY - 3), 6.0f, 6.0f);

                    // Module name label
                    g.setColour(dotCol.withAlpha(0.7f));
                    auto name = InsertProcessor::getModuleName(processor_.insertSlots_[i].getModuleType());
                    g.drawText(name, cx + 8, dotY - 6, sectionWidth - cx - 14, 12,
                               juce::Justification::centredLeft);
                }
                else
                {
                    g.setColour(juce::Colour(0xFF222228));
                    g.fillEllipse((float)(cx - 2), (float)(dotY - 2), 4.0f, 4.0f);
                }
            }
        }

        // Drag highlight on target slot
        if (dragActive_ && dragTargetSlot_ >= 0 && dragTargetSlot_ < numSlots)
        {
            auto tb = slotBtns_[(size_t)dragTargetSlot_].getBounds();
            g.setColour(juce::Colour(0xFFf97316).withAlpha(0.4f));
            g.drawRect(tb, 2);
        }

        // Highlight source slot being dragged
        if (dragActive_ && dragSourceSlot_ >= 0 && dragSourceSlot_ < numSlots)
        {
            auto sb = slotBtns_[(size_t)dragSourceSlot_].getBounds();
            g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.15f));
            g.fillRect(sb);
        }
    }

    void resizeSectionContent() override
    {
        int y = getContentStartY() + 20; // after "INSERTS" header

        // 2 columns × 5 rows of slot buttons
        constexpr int slotH = 22;
        constexpr int slotGap = 2;
        constexpr int cols = 2;
        constexpr int rows = 5;
        int totalW = sectionWidth - 20; // 10px padding each side
        int colW = (totalW - 2) / cols; // 2px gap between columns
        int startX = 10;

        for (int col = 0; col < cols; ++col)
        {
            for (int row = 0; row < rows; ++row)
            {
                int slotIdx = col * rows + row;
                if (slotIdx < numSlots)
                {
                    int sx = startX + col * (colW + 2);
                    int sy = y + row * (slotH + slotGap);
                    slotBtns_[slotIdx].setBounds(sx, sy, colW, slotH);
                }
            }
        }

        int slotsBottom = y + rows * (slotH + slotGap);

        // PRE / POST buttons
        int btnY = slotsBottom + 24; // after divider + ROUTING header
        int btnW = (sectionWidth - 24) / 2;
        preBtn_.setBounds(10, btnY, btnW, 22);
        postBtn_.setBounds(10 + btnW + 4, btnY, btnW, 22);
    }

private:
    BZideProcessor& processor_;

    static constexpr int numSlots = BZideProcessor::numInsertSlots;
    std::array<juce::TextButton, numSlots> slotBtns_;

    juce::TextButton preBtn_  { "PRE" };
    juce::TextButton postBtn_ { "POST" };

    // Slot drag state
    int dragSourceSlot_ = -1;
    int dragTargetSlot_ = -1;
    bool dragActive_ = false;

    void onSlotClicked(int slotIndex)
    {
        auto& slot = processor_.insertSlots_[slotIndex];

        if (slot.isLoaded())
        {
            // Toggle bypass on left-click
            bool newBypass = !slot.isBypassed();
            processor_.setInsertBypass(slotIndex, newBypass);
            syncSlotsFromProcessor();
        }
        else
        {
            // Show module picker menu
            juce::PopupMenu menu;
            menu.addItem(1, "Saturation");
            menu.addItem(2, "Compressor");
            menu.addItem(3, "Gate");
            menu.addItem(4, "De-Esser");
            menu.addItem(5, "Limiter");
            menu.addItem(6, "LA-2A");

            menu.showMenuAsync(
                juce::PopupMenu::Options().withTargetComponent(&slotBtns_[(size_t)slotIndex]),
                [this, slotIndex](int result)
                {
                    if (result > 0)
                    {
                        processor_.loadInsert(slotIndex,
                            static_cast<InsertProcessor::ModuleType>(result));
                        syncSlotsFromProcessor();
                        repaint();
                    }
                });
        }
    }

    void showSlotContextMenu(int slotIndex)
    {
        auto& slot = processor_.insertSlots_[slotIndex];
        if (!slot.isLoaded()) return;

        juce::PopupMenu menu;
        menu.addItem(1, slot.isBypassed() ? "Enable" : "Bypass");
        menu.addItem(2, "Remove");
        menu.addSeparator();

        // Move options
        if (slotIndex > 0)
            menu.addItem(10, "Move Up");
        if (slotIndex < numSlots - 1)
            menu.addItem(11, "Move Down");

        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(&slotBtns_[(size_t)slotIndex]),
            [this, slotIndex](int result)
            {
                if (result == 1)
                {
                    bool newBypass = !processor_.insertSlots_[slotIndex].isBypassed();
                    processor_.setInsertBypass(slotIndex, newBypass);
                }
                else if (result == 2)
                {
                    processor_.removeInsert(slotIndex);
                }
                else if (result == 10)
                {
                    processor_.swapInserts(slotIndex, slotIndex - 1);
                }
                else if (result == 11)
                {
                    processor_.swapInserts(slotIndex, slotIndex + 1);
                }
                syncSlotsFromProcessor();
                repaint();
            });
    }
};

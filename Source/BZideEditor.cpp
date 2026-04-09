#include "BZideEditor.h"

BZideEditor::BZideEditor(BZideProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);
    startTimerHz(30);

    auto& apvts = processor.getAPVTS();

    // Create all section components
    preSection = std::make_unique<PreSection>(apvts);
    eqSection = std::make_unique<EQSection>(apvts, &processor);
    ds2Section = std::make_unique<DS2Section>(apvts);
    compSection = std::make_unique<CompSection>(apvts);
    gateSection = std::make_unique<GateSection>(apvts);
    insertSection = std::make_unique<InsertSection>(p);
    outputSection = std::make_unique<OutputSection>(processor, apvts);

    // Set LookAndFeel on each section
    preSection->setLookAndFeel(&lnf);
    eqSection->setLookAndFeel(&lnf);
    ds2Section->setLookAndFeel(&lnf);
    compSection->setLookAndFeel(&lnf);
    gateSection->setLookAndFeel(&lnf);
    insertSection->setLookAndFeel(&lnf);
    outputSection->setLookAndFeel(&lnf);

    // Add all sections as visible children
    addAndMakeVisible(preSection.get());
    addAndMakeVisible(eqSection.get());
    addAndMakeVisible(ds2Section.get());
    addAndMakeVisible(compSection.get());
    addAndMakeVisible(gateSection.get());
    addAndMakeVisible(insertSection.get());
    addAndMakeVisible(outputSection.get());

    // Wire drag callbacks for all draggable sections
    auto setupDrag = [this](ChannelSection* sec) {
        sec->onDragMove = [this](ChannelSection* s, int delta) { handleDragMove(s, delta); };
        sec->onDragEnd = [this](ChannelSection* s) { handleDragEnd(s); };
    };

    setupDrag(preSection.get());
    setupDrag(eqSection.get());
    setupDrag(ds2Section.get());
    setupDrag(compSection.get());
    setupDrag(gateSection.get());
    setupDrag(insertSection.get());

    // Initialize draggable order from processor
    for (int i = 0; i < 6; ++i)
        draggableOrder[(size_t)i] = static_cast<SectionId>(processor.sectionOrder_[i].load());

    // Set size LAST — this triggers resized() which needs sections to exist
    int totalWidth = kSectionWidth * 6 + kOutputWidth;
    setSize(totalWidth, kTotalHeight);

    // Initialize A/B slots with current state
    saveCurrentToSlot(abSlotA);
    saveCurrentToSlot(abSlotB);

    // Init preset names (not full state presets - just names for the menu)
    initPresets();

    // Style popup menu
    getLookAndFeel().setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFF18181C));
    getLookAndFeel().setColour(juce::PopupMenu::textColourId, juce::Colours::white.withAlpha(0.85f));
    getLookAndFeel().setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xFF8B1515));
    getLookAndFeel().setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    getLookAndFeel().setColour(juce::PopupMenu::headerTextColourId, juce::Colour(0xFFDD6600));
}

BZideEditor::~BZideEditor()
{
    // Clear LookAndFeel on sections before destructor
    preSection->setLookAndFeel(nullptr);
    eqSection->setLookAndFeel(nullptr);
    ds2Section->setLookAndFeel(nullptr);
    compSection->setLookAndFeel(nullptr);
    gateSection->setLookAndFeel(nullptr);
    insertSection->setLookAndFeel(nullptr);
    outputSection->setLookAndFeel(nullptr);

    setLookAndFeel(nullptr);
    stopTimer();
}

ChannelSection* BZideEditor::getSectionById(SectionId id)
{
    switch (id)
    {
        case SectionId::PRE:    return preSection.get();
        case SectionId::EQ:     return eqSection.get();
        case SectionId::DS2:    return ds2Section.get();
        case SectionId::COMP:   return compSection.get();
        case SectionId::GATE:   return gateSection.get();
        case SectionId::INSERT: return insertSection.get();
        case SectionId::OUTPUT: return outputSection.get();
        default: return nullptr;
    }
}

int BZideEditor::findDragIndex(SectionId id)
{
    for (int i = 0; i < 6; ++i)
        if (draggableOrder[(size_t)i] == id)
            return i;
    return -1;
}

void BZideEditor::handleDragMove(ChannelSection* section, int deltaX)
{
    int idx = findDragIndex(section->getSectionId());
    if (idx < 0) return;

    int threshold = kSectionWidth / 2;

    auto doSwap = [&](int fromIdx, int toIdx) {
        std::swap(draggableOrder[(size_t)fromIdx], draggableOrder[(size_t)toIdx]);

        // Clear all highlights
        for (int i = 0; i < 6; ++i)
            getSectionById(draggableOrder[(size_t)i])->setDragHighlight(false);

        // Highlight the dragged section in its new position
        section->setDragHighlight(true);

        // Update processor order
        std::array<int, 6> order;
        for (int i = 0; i < 6; ++i)
            order[(size_t)i] = static_cast<int>(draggableOrder[(size_t)i]);
        processor.setSectionOrder(order);

        // Reset drag origin so delta starts fresh from current mouse position
        section->resetDragOrigin();

        resized();
        repaint();
    };

    if (deltaX > threshold && idx < 5)
        doSwap(idx, idx + 1);
    else if (deltaX < -threshold && idx > 0)
        doSwap(idx, idx - 1);
}

void BZideEditor::handleDragEnd(ChannelSection*)
{
    // Clear all drag highlights
    for (int i = 0; i < 6; ++i)
        getSectionById(draggableOrder[(size_t)i])->setDragHighlight(false);
}

// ── A/B Comparison ──
void BZideEditor::saveCurrentToSlot(juce::XmlElement& slot)
{
    slot.removeAllAttributes();
    for (auto* param : processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            slot.setAttribute(p->getParameterID(), (double)p->getValue());
    }
}

void BZideEditor::loadSlotToCurrent(const juce::XmlElement& slot)
{
    for (auto* param : processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            if (slot.hasAttribute(p->getParameterID()))
                p->setValueNotifyingHost((float)slot.getDoubleAttribute(p->getParameterID()));
        }
    }
}

// ── Preset System ──
void BZideEditor::initPresets()
{
    presetNames.clear();
    presetNames.push_back({"Default", "Init"});
    presetNames.push_back({"Vocals", "Vocal Chain - Clean"});
    presetNames.push_back({"Vocals", "Vocal Chain - Warm"});
    presetNames.push_back({"Vocals", "Vocal Chain - Aggressive"});
    presetNames.push_back({"Vocals", "De-Ess + Compress"});
    presetNames.push_back({"Drums", "Drum Bus Glue"});
    presetNames.push_back({"Drums", "Kick Punch"});
    presetNames.push_back({"Drums", "Snare Snap"});
    presetNames.push_back({"Drums", "Parallel Crush"});
    presetNames.push_back({"Bass", "Bass Smooth"});
    presetNames.push_back({"Bass", "Bass Aggressive"});
    presetNames.push_back({"Guitar", "Acoustic Guitar"});
    presetNames.push_back({"Guitar", "Electric Clean"});
    presetNames.push_back({"Mix Bus", "Stereo Bus Gentle"});
    presetNames.push_back({"Mix Bus", "Stereo Bus Medium"});
    presetNames.push_back({"Mix Bus", "Mix Gel"});
    presetNames.push_back({"Mastering", "Transparent"});
    presetNames.push_back({"Mastering", "Warm Analog"});
    presetNames.push_back({"Mastering", "Loud & Proud"});
    presetNames.push_back({"Creative", "Lo-Fi Crush"});
    presetNames.push_back({"Creative", "Tape Saturator"});
    currentPreset = 0;
}

void BZideEditor::loadPreset(int index)
{
    if (index >= 0 && index < (int)presetNames.size())
    {
        currentPreset = index;
        repaint();
    }
}

void BZideEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    juce::String lastCat;
    int id = 1;
    for (auto& p : presetNames)
    {
        if (p.category != lastCat)
        {
            if (lastCat.isNotEmpty()) menu.addSeparator();
            menu.addSectionHeader(p.category);
            lastCat = p.category;
        }
        menu.addItem(id, p.name, true, id - 1 == currentPreset);
        id++;
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
        localAreaToGlobal(presetNameArea).toFloat().toNearestInt()),
        [this](int result) {
            if (result > 0) {
                currentPreset = result - 1;
                repaint();
            }
        });
}

// ── Mouse handling for top/bottom bar ──
void BZideEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();

    if (prevPresetBtn.expanded(2).contains(pos))
    {
        if (currentPreset > 0) { currentPreset--; repaint(); }
        return;
    }
    if (nextPresetBtn.expanded(2).contains(pos))
    {
        if (currentPreset < (int)presetNames.size() - 1) { currentPreset++; repaint(); }
        return;
    }
    if (presetNameArea.expanded(2).contains(pos))
    {
        showPresetMenu();
        return;
    }
    if (inBtn.expanded(2).contains(pos))
    {
        inActive = !inActive;
        repaint();
        return;
    }
    if (aBtn.expanded(2).contains(pos))
    {
        if (abState == 1) saveCurrentToSlot(abSlotB);
        abState = 0;
        loadSlotToCurrent(abSlotA);
        repaint();
        return;
    }
    if (bBtn.expanded(2).contains(pos))
    {
        if (abState == 0) saveCurrentToSlot(abSlotA);
        abState = 1;
        loadSlotToCurrent(abSlotB);
        repaint();
        return;
    }
    if (copyBtn.expanded(2).contains(pos))
    {
        auto& slot = (abState == 0) ? abSlotA : abSlotB;
        saveCurrentToSlot(slot);
        repaint();
        return;
    }
    if (pasteBtn.expanded(2).contains(pos))
    {
        auto& slot = (abState == 0) ? abSlotA : abSlotB;
        loadSlotToCurrent(slot);
        repaint();
        return;
    }
}

void BZideEditor::timerCallback()
{
    // Update VU meter with output level and repaint
    outputSection->updateMeter();
    outputSection->repaint();

    // Sync insert slot UI with processor state
    insertSection->syncSlotsFromProcessor();
}

void BZideEditor::paint(juce::Graphics& g)
{
    // Black background behind everything
    g.fillAll(juce::Colours::black);

    // Trial banner
    auto status = processor.getLicenseStatus();
    if (status != LicenseValidator::Status::Active)
    {
        juce::String text;
        juce::Colour color;
        switch (status)
        {
            case LicenseValidator::Status::Trial:
                text = "Trial: " + juce::String(processor.getTrialDaysRemaining()) + " days remaining";
                color = juce::Colour(0xFFf59e0b); break;
            case LicenseValidator::Status::Expired:
                text = "Trial Expired";
                color = juce::Colour(0xFFDD2200); break;
            default:
                text = "Not Activated";
                color = juce::Colour(0xFF666666); break;
        }
        auto banner = getLocalBounds().removeFromBottom(20);
        g.setColour(juce::Colour(0xDD000000));
        g.fillRect(banner);
        g.setColour(color);
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        g.drawText(text, banner, juce::Justification::centred);
    }
}

void BZideEditor::paintOverChildren(juce::Graphics& g)
{
    // ── Bottom Preset/A/B Bar ──
    {
        auto barArea = juce::Rectangle<int>(0, getHeight() - 24, getWidth(), 24);
        g.setColour(juce::Colour(0xF0101014));
        g.fillRect(barArea);
        g.setColour(juce::Colour(0xFF2A2A30));
        g.drawHorizontalLine(barArea.getY(), 0, (float)getWidth());

        float bh = 16.0f;
        float by = (float)barArea.getY() + ((float)barArea.getHeight() - bh) / 2.0f;

        auto drawBarBtn = [&](juce::Rectangle<float> r, const juce::String& text, bool active, juce::Colour activeCol) {
            if (active) {
                g.setColour(activeCol);
                g.fillRoundedRectangle(r, 3.0f);
                g.setColour(juce::Colours::white);
            } else {
                g.setColour(juce::Colour(0xFF333338));
                g.fillRoundedRectangle(r, 3.0f);
                g.setColour(juce::Colour(0xFF888888));
            }
            g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
            g.drawText(text, r.toNearestInt(), juce::Justification::centred);
        };

        float x = 8.0f;
        float gap = 3.0f;

        // Prev preset
        auto prevR = juce::Rectangle<float>(x, by, 18.0f, bh);
        drawBarBtn(prevR, "<", false, juce::Colour(0xFF666666));
        prevPresetBtn = prevR.toNearestInt();
        x += 18.0f + gap;

        // Preset name
        float presetW = 120.0f;
        auto nameR = juce::Rectangle<float>(x, by, presetW, bh);
        g.setColour(juce::Colour(0xFF1A1A1E));
        g.fillRoundedRectangle(nameR, 3.0f);
        g.setColour(juce::Colour(0xFFCCCCCC));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        juce::String pName = (currentPreset >= 0 && currentPreset < (int)presetNames.size())
            ? presetNames[(size_t)currentPreset].name : "Init";
        g.drawText(pName, nameR.toNearestInt(), juce::Justification::centred);
        presetNameArea = nameR.toNearestInt();
        x += presetW + gap;

        // Next preset
        auto nextR = juce::Rectangle<float>(x, by, 18.0f, bh);
        drawBarBtn(nextR, ">", false, juce::Colour(0xFF666666));
        nextPresetBtn = nextR.toNearestInt();
        x += 18.0f + 12.0f;

        // IN button
        auto inR = juce::Rectangle<float>(x, by, 24.0f, bh);
        drawBarBtn(inR, "IN", inActive, juce::Colour(0xFF44BB44));
        inBtn = inR.toNearestInt();
        x += 24.0f + gap;

        // A button
        auto aR = juce::Rectangle<float>(x, by, 22.0f, bh);
        drawBarBtn(aR, "A", abState == 0, juce::Colour(0xFF4488FF));
        aBtn = aR.toNearestInt();
        x += 22.0f + gap;

        // B button
        auto bR = juce::Rectangle<float>(x, by, 22.0f, bh);
        drawBarBtn(bR, "B", abState == 1, juce::Colour(0xFF4488FF));
        bBtn = bR.toNearestInt();
        x += 22.0f + 8.0f;

        // COPY
        auto copyR = juce::Rectangle<float>(x, by, 36.0f, bh);
        drawBarBtn(copyR, "COPY", false, juce::Colour(0xFF666666));
        copyBtn = copyR.toNearestInt();
        x += 36.0f + gap;

        // PASTE
        auto pasteR = juce::Rectangle<float>(x, by, 38.0f, bh);
        drawBarBtn(pasteR, "PASTE", false, juce::Colour(0xFF666666));
        pasteBtn = pasteR.toNearestInt();
    }
}

void BZideEditor::resized()
{
    int x = 0;

    // Layout the 6 draggable sections in current order
    for (int i = 0; i < 6; ++i)
    {
        auto* sec = getSectionById(draggableOrder[(size_t)i]);
        sec->setBounds(x, 0, kSectionWidth, kTotalHeight);
        x += kSectionWidth;
    }

    // OUTPUT section (always last)
    outputSection->setBounds(x, 0, kOutputWidth, kTotalHeight);
}

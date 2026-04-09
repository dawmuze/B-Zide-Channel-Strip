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
        auto* p = processor.getAPVTS().getParameter("master_bypass");
        p->setValueNotifyingHost(inActive ? 1.0f : 0.0f); // toggle: IN was active, now bypass
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
    if (buyBtn.expanded(4).contains(pos))
    {
        juce::URL("https://dawmuze.com/purchase").launchInDefaultBrowser();
        return;
    }
    if (infoBtn.expanded(4).contains(pos))
    {
        juce::URL("https://dawmuze.com/products/bzide-channel-strip").launchInDefaultBrowser();
        return;
    }
}

void BZideEditor::timerCallback()
{
    // Update VU meter (partial repaint handled inside updateMeter)
    outputSection->updateMeter();

    // Sync insert slot UI with processor state
    insertSection->syncSlotsFromProcessor();
}

void BZideEditor::paint(juce::Graphics& g)
{
    // Black background behind everything
    g.fillAll(juce::Colours::black);
}

void BZideEditor::paintOverChildren(juce::Graphics& g)
{
    // ── Bottom License/Status Bar (KI-2A style) ──
    {
        int botH = 22;
        auto barArea = juce::Rectangle<int>(0, getHeight() - botH, getWidth(), botH);

        // Dark gradient background
        juce::ColourGradient botGrad(
            juce::Colour(0xFF0E0E12), 0.0f, (float)barArea.getY(),
            juce::Colour(0xFF18181C), 0.0f, (float)barArea.getBottom(), false);
        g.setGradientFill(botGrad);
        g.fillRect(barArea);

        // Top separator
        g.setColour(juce::Colour(0xFF333338));
        g.drawHorizontalLine(barArea.getY(), 0.0f, (float)getWidth());

        // Left side — Status LED + text
        float ledX = 12.0f;
        float ledCY = (float)barArea.getCentreY();

        auto status = processor.getLicenseStatus();

        // Green LED dot
        juce::Colour ledCol;
        switch (status)
        {
            case LicenseValidator::Status::Active: ledCol = juce::Colour(0xFF44DD44); break;
            case LicenseValidator::Status::Trial:  ledCol = juce::Colour(0xFF44DD44); break;
            case LicenseValidator::Status::Expired: ledCol = juce::Colour(0xFFef4444); break;
            default: ledCol = juce::Colour(0xFF666666); break;
        }
        g.setColour(ledCol.withAlpha(0.15f));
        g.fillEllipse(ledX - 5.0f, ledCY - 5.0f, 10.0f, 10.0f);
        g.setColour(ledCol);
        g.fillEllipse(ledX - 3.0f, ledCY - 3.0f, 6.0f, 6.0f);

        // Status text
        float textX = ledX + 8.0f;
        if (status == LicenseValidator::Status::Trial)
        {
            g.setColour(juce::Colour(0xFFf97316));
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText("trial", (int)textX, barArea.getY(), 30, botH, juce::Justification::centredLeft);
            g.setColour(juce::Colour(0xFF888890));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            g.drawText("- " + juce::String(processor.getTrialDaysRemaining()) + " days",
                       (int)(textX + 32), barArea.getY(), 80, botH, juce::Justification::centredLeft);
        }
        else if (status == LicenseValidator::Status::Active)
        {
            g.setColour(juce::Colour(0xFF44DD44));
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText("licensed", (int)textX, barArea.getY(), 60, botH, juce::Justification::centredLeft);
        }
        else if (status == LicenseValidator::Status::Expired)
        {
            g.setColour(juce::Colour(0xFFef4444));
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText("expired", (int)textX, barArea.getY(), 60, botH, juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(juce::Colour(0xFF888890));
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText("not activated", (int)textX, barArea.getY(), 90, botH, juce::Justification::centredLeft);
        }

        // Right side — BUY button
        int buyW = 36, buyH2 = 14;
        int buyX = getWidth() - buyW - 8;
        int buyY = barArea.getCentreY() - buyH2 / 2;
        auto buyR = juce::Rectangle<float>((float)buyX, (float)buyY, (float)buyW, (float)buyH2);
        g.setColour(juce::Colour(0xFF8B1515).withAlpha(0.4f));
        g.fillRoundedRectangle(buyR, 2.0f);
        g.setColour(juce::Colour(0xFFDD2200));
        g.drawRoundedRectangle(buyR, 2.0f, 0.8f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("BUY", buyR.toNearestInt(), juce::Justification::centred);
        buyBtn = buyR.toNearestInt();

        // INFO button (left of BUY)
        int infoX = buyX - buyW - 4;
        auto infoR = juce::Rectangle<float>((float)infoX, (float)buyY, (float)buyW, (float)buyH2);
        g.setColour(juce::Colour(0xFF222228));
        g.fillRoundedRectangle(infoR, 2.0f);
        g.setColour(juce::Colour(0xFF444448));
        g.drawRoundedRectangle(infoR, 2.0f, 0.6f);
        g.setColour(juce::Colour(0xFF888890));
        g.setFont(juce::Font(juce::FontOptions(9.0f)).boldened());
        g.drawText("INFO", infoR.toNearestInt(), juce::Justification::centred);
        infoBtn = infoR.toNearestInt();
    }

    // ── Top Preset/A/B Bar (KI-2A style) ──
    {
        auto barArea = juce::Rectangle<int>(0, 0, getWidth(), 26);

        // Dark gradient background
        juce::ColourGradient topGrad(
            juce::Colour(0xFF18181C), 0.0f, 0.0f,
            juce::Colour(0xFF0E0E12), 0.0f, 26.0f, false);
        g.setGradientFill(topGrad);
        g.fillRect(barArea);

        // Bottom separator
        g.setColour(juce::Colour(0xFF333338));
        g.drawHorizontalLine(barArea.getBottom() - 1, 0, (float)getWidth());

        float bh = 18.0f;
        float by = (float)barArea.getY() + ((float)barArea.getHeight() - bh) / 2.0f;

        auto drawBarBtn = [&](juce::Rectangle<float> r, const juce::String& text, bool active, juce::Colour activeCol) {
            if (active) {
                g.setColour(activeCol.withAlpha(0.25f));
                g.fillRoundedRectangle(r, 3.0f);
                g.setColour(activeCol);
                g.drawRoundedRectangle(r, 3.0f, 1.0f);
                g.setColour(activeCol);
            } else {
                g.setColour(juce::Colour(0xFF222228));
                g.fillRoundedRectangle(r, 3.0f);
                g.setColour(juce::Colour(0xFF444448));
                g.drawRoundedRectangle(r, 3.0f, 0.6f);
                g.setColour(juce::Colour(0xFF888890));
            }
            g.setFont(juce::Font(juce::FontOptions(10.0f)).boldened());
            g.drawText(text, r.toNearestInt(), juce::Justification::centred);
        };

        float gap = 3.0f;
        float w = (float)getWidth();

        // ── LEFT: < > nav arrows ──
        float lx = 8.0f;
        auto prevR = juce::Rectangle<float>(lx, by, 22.0f, bh);
        drawBarBtn(prevR, "<", false, juce::Colour(0xFF555558));
        prevPresetBtn = prevR.toNearestInt();
        lx += 22.0f + gap;

        auto nextR = juce::Rectangle<float>(lx, by, 22.0f, bh);
        drawBarBtn(nextR, ">", false, juce::Colour(0xFF555558));
        nextPresetBtn = nextR.toNearestInt();

        // ── CENTER: Preset name ──
        float presetW = 180.0f;
        float presetX = (w - presetW) / 2.0f;
        auto nameR = juce::Rectangle<float>(presetX, by, presetW, bh);
        g.setColour(juce::Colour(0xFF111115));
        g.fillRoundedRectangle(nameR, 3.0f);
        g.setColour(juce::Colour(0xFF3A3A40));
        g.drawRoundedRectangle(nameR, 3.0f, 0.6f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());
        juce::String pName = (currentPreset >= 0 && currentPreset < (int)presetNames.size())
            ? presetNames[(size_t)currentPreset].name : "Init";
        g.drawText(pName, nameR.toNearestInt(), juce::Justification::centred);
        presetNameArea = nameR.toNearestInt();

        // ── RIGHT: IN, A, B, COPY, PASTE (right-to-left) ──
        float rx = w - 8.0f;

        // PASTE
        rx -= 36.0f;
        auto pasteR = juce::Rectangle<float>(rx, by, 36.0f, bh);
        drawBarBtn(pasteR, "PASTE", false, juce::Colour(0xFF555558));
        pasteBtn = pasteR.toNearestInt();

        // COPY
        rx -= 36.0f + gap;
        auto copyR = juce::Rectangle<float>(rx, by, 36.0f, bh);
        drawBarBtn(copyR, "COPY", false, juce::Colour(0xFF555558));
        copyBtn = copyR.toNearestInt();

        rx -= 10.0f;

        // B
        rx -= 22.0f;
        auto bR = juce::Rectangle<float>(rx, by, 22.0f, bh);
        drawBarBtn(bR, "B", abState == 1, juce::Colour(0xFF4488FF));
        bBtn = bR.toNearestInt();

        // A
        rx -= 22.0f + gap;
        auto aR = juce::Rectangle<float>(rx, by, 22.0f, bh);
        drawBarBtn(aR, "A", abState == 0, juce::Colour(0xFF4488FF));
        aBtn = aR.toNearestInt();

        rx -= 10.0f;

        // IN
        rx -= 24.0f;
        auto inR = juce::Rectangle<float>(rx, by, 24.0f, bh);
        drawBarBtn(inR, "IN", inActive, juce::Colour(0xFF44DD44));
        inBtn = inR.toNearestInt();
    }
}

void BZideEditor::resized()
{
    int x = 0;

    // Top bar = 26px, sections start below
    int topBarH = 26;

    // Layout the 6 draggable sections in current order
    for (int i = 0; i < 6; ++i)
    {
        auto* sec = getSectionById(draggableOrder[(size_t)i]);
        sec->setBounds(x, topBarH, kSectionWidth, kTotalHeight - topBarH);
        x += kSectionWidth;
    }

    // OUTPUT section (always last)
    outputSection->setBounds(x, topBarH, kOutputWidth, kTotalHeight - topBarH);
}

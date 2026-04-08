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
    insertSection = std::make_unique<InsertSection>();
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

    // Initialize draggable order from processor
    for (int i = 0; i < 5; ++i)
        draggableOrder[(size_t)i] = static_cast<SectionId>(processor.sectionOrder_[i].load());

    // Set size LAST — this triggers resized() which needs sections to exist
    int totalWidth = kSectionWidth * 6 + kOutputWidth;
    setSize(totalWidth, kTotalHeight);
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
    for (int i = 0; i < 5; ++i)
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
        for (int i = 0; i < 5; ++i)
            getSectionById(draggableOrder[(size_t)i])->setDragHighlight(false);

        // Highlight the dragged section in its new position
        section->setDragHighlight(true);

        // Update processor order
        std::array<int, 5> order;
        for (int i = 0; i < 5; ++i)
            order[(size_t)i] = static_cast<int>(draggableOrder[(size_t)i]);
        processor.setSectionOrder(order);

        // Reset drag origin so delta starts fresh from current mouse position
        section->resetDragOrigin();

        resized();
        repaint();
    };

    if (deltaX > threshold && idx < 4)
        doSwap(idx, idx + 1);
    else if (deltaX < -threshold && idx > 0)
        doSwap(idx, idx - 1);
}

void BZideEditor::handleDragEnd(ChannelSection*)
{
    // Clear all drag highlights
    for (int i = 0; i < 5; ++i)
        getSectionById(draggableOrder[(size_t)i])->setDragHighlight(false);
}

void BZideEditor::timerCallback()
{
    // Sync LED state with limiter button
    outputSection->getLED().setOn(outputSection->getLimiterButton().getToggleState());

    // Update VU meter with output level and repaint
    outputSection->updateMeter();
    outputSection->repaint();
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

void BZideEditor::paintOverChildren(juce::Graphics&)
{
    // LED bombillas removed — bypass state shown by ON button color
}

void BZideEditor::resized()
{
    int x = 0;

    // Layout the 5 draggable sections in current order
    for (int i = 0; i < 5; ++i)
    {
        auto* sec = getSectionById(draggableOrder[(size_t)i]);
        sec->setBounds(x, 0, kSectionWidth, kTotalHeight);
        x += kSectionWidth;
    }

    // INSERT section (always position 6)
    insertSection->setBounds(x, 0, kSectionWidth, kTotalHeight);
    x += kSectionWidth;

    // OUTPUT section (always last)
    outputSection->setBounds(x, 0, kOutputWidth, kTotalHeight);
}

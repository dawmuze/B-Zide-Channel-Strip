#include "MeterChannel.h"

MeterChannel::MeterChannel()
{
    addAndMakeVisible (meterFace);
}

void MeterChannel::setLevel (float dB)
{
    meterFace.setLevel (dB);
}

void MeterChannel::setMeterMode (MeterMode mode)
{
    meterFace.setMeterMode (mode);
}

void MeterChannel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF2A2A2E));
}

void MeterChannel::resized()
{
    meterFace.setBounds (getLocalBounds().reduced (2));
}

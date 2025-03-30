#pragma once
#include <JuceHeader.h>
#include <cmath>

class TunerComponent : public juce::Component, private juce::Timer
{
public:
    TunerComponent()
    {
        startTimerHz(30); // Redraw at 30 fps
    }

    void setFrequency(float freqInHz)
    {
        currentFrequency = freqInHz;
        updateTuningInfo();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        auto bounds = getLocalBounds();
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();

        // Draw center line
        g.setColour(juce::Colours::white);
        g.drawLine(centerX, centerY - 30, centerX, centerY + 30, 2.0f);

        // Draw needle
        float maxOffset = 100.0f; // pixels left/right
        float normalizedOffset = juce::jlimit(-1.0f, 1.0f, centsOffset / 50.0f);
        float needleX = centerX + normalizedOffset * maxOffset;

        g.setColour(std::abs(centsOffset) < 5.0f ? juce::Colours::green : juce::Colours::red);
        g.drawLine(needleX, centerY - 50, needleX, centerY + 50, 4.0f);

        // Note label
        g.setColour(juce::Colours::white);
        g.setFont(30.0f);
        g.drawText(noteName, 0, 10, bounds.getWidth(), 40, juce::Justification::centred);

        // Cents offset
        g.setFont(18.0f);
        g.drawText(juce::String(centsOffset, 1) + " cents",
                   0, bounds.getBottom() - 30, bounds.getWidth(), 20,
                   juce::Justification::centred);
    }

private:
    float currentFrequency = 0.0f;
    float centsOffset = 0.0f;
    juce::String noteName = "-";

    void timerCallback() override
    {
        repaint();
    }

    void updateTuningInfo()
    {
        static const juce::String noteNames[] =
            { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        if (currentFrequency < 20.0f || currentFrequency > 2000.0f)
        {
            noteName = "-";
            centsOffset = 0.0f;
            return;
        }

        float midiNote = 69 + 12.0f * std::log2(currentFrequency / 440.0f);
        int roundedMidi = std::round(midiNote);
        float noteFreq = 440.0f * std::pow(2.0f, (roundedMidi - 69) / 12.0f);

        noteName = noteNames[roundedMidi % 12] + juce::String(roundedMidi / 12 - 1);
        centsOffset = 1200.0f * std::log2(currentFrequency / noteFreq);
    }
};

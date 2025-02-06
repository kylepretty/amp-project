#pragma once
#include <JuceHeader.h>

class FrequencyResponseComponent : public juce::Component
{
public:
    FrequencyResponseComponent() {}

    void prepare(const juce::dsp::ProcessSpec&) {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::green);
        plotFrequencyResponse(g);
    }

    void plotFrequencyResponse(juce::Graphics& g)
    {
        const int width = getWidth();
        const int height = getHeight();

        juce::Path responseCurve;
        responseCurve.startNewSubPath(0, height);

        for (int x = 1; x < width; ++x)
        {
            float freq = juce::jmap<float>(x, 0, width, 20.0f, 20000.0f);
            float gain = juce::Decibels::gainToDecibels(1.0f); // Placeholder
            float y = height - juce::jmap<float>(gain, -48.0f, 12.0f, height, 0.0f);
            responseCurve.lineTo(x, y);
        }

        g.strokePath(responseCurve, juce::PathStrokeType(1.0f));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponseComponent)
};

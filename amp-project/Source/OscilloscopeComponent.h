#pragma once
#include <JuceHeader.h>

class OscilloscopeComponent : public juce::Component
{
public:
    OscilloscopeComponent(const juce::String& title) : titleText(title)
    {
        setOpaque(true);
    }

    void prepare(int bufferSize)
    {
        samples.ensureStorageAllocated(bufferSize);
        samples.clear();
    }

    void pushSamples(const float* newSamples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            samples.add(newSamples[i]);

        while (samples.size() > getWidth())
            samples.remove(0);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);

        g.drawText(titleText, getLocalBounds().reduced(4), juce::Justification::topLeft);

        if (samples.isEmpty())
            return;

        juce::Path waveform;
        waveform.startNewSubPath(0, getHeight() / 2.0f);

        for (int x = 0; x < samples.size(); ++x)
        {
            float y = juce::jmap(samples[x], -1.0f, 1.0f, (float)getHeight(), 0.0f);
            waveform.lineTo(x, y);
        }

        g.strokePath(waveform, juce::PathStrokeType(1.0f));
    }

private:
    juce::String titleText;
    juce::Array<float> samples;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeComponent)
};

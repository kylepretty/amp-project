#pragma once
#include <JuceHeader.h>
#include "AmpProfile.h"

class WaveshaperProcessor
{
public:
    WaveshaperProcessor()
    {
        // Default waveshaping functions (non-capturing lambdas convert to function pointers)
        waveShaperPreEQ.functionToUse = [](float sample) { return sample / (std::abs(sample) + 1.0f); };
        waveShaperPostEQ.functionToUse = [](float sample) { return sample / (std::abs(sample) + 1.0f); };
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        waveShaperPreEQ.prepare(spec);
        waveShaperPostEQ.prepare(spec);
    }

    // Use function pointer type to match WaveShaper's functionToUse
    void setPreEQWaveshaperFunction(float (*newFunction)(float))
    {
        waveShaperPreEQ.functionToUse = newFunction; // Line 28
    }

    void setPostEQWaveshaperFunction(float (*newFunction)(float))
    {
        waveShaperPostEQ.functionToUse = newFunction; // Line 33
    }

    void processPreEQ(juce::dsp::AudioBlock<float>& block)
    {
        auto* leftChannel = block.getChannelPointer(0);
        auto* rightChannel = block.getChannelPointer(1);
        for (int sample = 0; sample < block.getNumSamples(); ++sample)
        {
            float processedSample = waveShaperPreEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }
    }

    void processPostEQ(juce::dsp::AudioBlock<float>& block)
    {
        auto* leftChannel = block.getChannelPointer(0);
        auto* rightChannel = block.getChannelPointer(1);
        for (int sample = 0; sample < block.getNumSamples(); ++sample)
        {
            float processedSample = waveShaperPostEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }
    }

private:
    juce::dsp::WaveShaper<float> waveShaperPreEQ;
    juce::dsp::WaveShaper<float> waveShaperPostEQ;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveshaperProcessor)
};
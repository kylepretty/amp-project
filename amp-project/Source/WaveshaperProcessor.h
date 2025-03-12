#pragma once
#include <JuceHeader.h>

class WaveshaperProcessor
{
public:
    WaveshaperProcessor()
    {
        waveShaperPreEQ.functionToUse = [](float sample) {
            return sample / (std::abs(sample) + 1.0f);
            };
        waveShaperPostEQ.functionToUse = [](float sample) {
            return sample / (std::abs(sample) + 1.0f);
            };
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        waveShaperPreEQ.prepare(spec);
        waveShaperPostEQ.prepare(spec);
    }

    void processPreEQ(juce::dsp::AudioBlock<float>& block)
    {
        auto* leftChannel = block.getChannelPointer(0);
        auto* rightChannel = block.getChannelPointer(1);
        for (int sample = 0; sample < block.getNumSamples(); ++sample) {
            float processedSample = waveShaperPreEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }
    }

    void processPostEQ(juce::dsp::AudioBlock<float>& block)
    {
        auto* leftChannel = block.getChannelPointer(0);
        auto* rightChannel = block.getChannelPointer(1);
        for (int sample = 0; sample < block.getNumSamples(); ++sample) {
            float processedSample = waveShaperPostEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }
    }

private:
    juce::dsp::WaveShaper<float> waveShaperPreEQ;
    juce::dsp::WaveShaper<float> waveShaperPostEQ;
};
#pragma once
#include <JuceHeader.h>

static float softClip(float sample) {
    return sample / (std::abs(sample) + 1.0f);
}

static float hardClip(float sample) {
    return std::max(-1.0f, std::min(1.0f, sample));
}

static float tanhClip(float sample) {
    return std::tanh(sample);
}

class WaveshaperProcessor {
public:
    WaveshaperProcessor() {
        setPreEQFunction(softClip);
        setPostEQFunction(softClip);
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        waveShaperPreEQ.prepare(spec);
        waveShaperPostEQ.prepare(spec);
    }

    void setPreEQFunction(float(*func)(float)) {
        waveShaperPreEQ.functionToUse = func;
    }

    void setPostEQFunction(float(*func)(float)) {
        waveShaperPostEQ.functionToUse = func;
    }

    void processPreEQ(juce::dsp::AudioBlock<float>& block) {
        auto* leftChannel = block.getChannelPointer(0);
        auto* rightChannel = block.getChannelPointer(1);
        for (int sample = 0; sample < block.getNumSamples(); ++sample) {
            float processedSample = waveShaperPreEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }
    }

    void processPostEQ(juce::dsp::AudioBlock<float>& block) {
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
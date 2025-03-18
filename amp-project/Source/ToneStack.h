#pragma once
#include <JuceHeader.h>

class ToneStack
{
public:
    ToneStack() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        low.prepare(spec);
        mid.prepare(spec);
        high.prepare(spec);

        updateFilters();
    }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        low.process(context);
        mid.process(context);
        high.process(context);
    }

    void setlowFrequency(float frequency) { lowFrequency = frequency; updateLow(); }
    void setlowQ(float q) { lowQ = q; updateLow(); }

    void setmidFrequency(float frequency) { midFrequency = frequency; updateMid(); }
    void setmidQ(float q) { midQ = q; updateMid(); }
    void updatemidGain(float gainDb) { midGain = juce::Decibels::decibelsToGain(gainDb); updateMid(); }

    void sethighFrequency(float frequency) { highFrequency = frequency; updateHigh(); }
    void sethighQ(float q) { highQ = q; updateHigh(); }
    void updatehighGain(float gainDb) { highGain = juce::Decibels::decibelsToGain(gainDb); updateHigh(); }

private:
    void updateFilters()
    {
        updateLow();
        updateMid();
        updateHigh();
    }

    void updateLow()
    {
        *low.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, lowFrequency, lowQ);
    }

    void updateMid()
    {
        *mid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, midFrequency, midQ, midGain);
    }

    void updateHigh()
    {
        *high.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highFrequency, highQ, highGain);
    }

    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> low;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> mid;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> high;

    // Filter parameter default values
    float lowFrequency = 15.0f;
    float lowQ = 0.23f;

    float midFrequency = 420.0f;
    float midQ = 0.29f;
    float midGain = -9.0f;

    float highFrequency = 415.0f;
    float highQ = 0.71f;
    float highGain = -0.5f;
};
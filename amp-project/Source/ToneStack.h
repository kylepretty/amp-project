#pragma once
#include <JuceHeader.h>

class ToneStack
{
public:
    ToneStack() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        lowCutFilter.prepare(spec);
        lowShelfFilter.prepare(spec);
        highShelfFilter800.prepare(spec);
        bellFilter800.prepare(spec);
        highShelfFilter1410.prepare(spec);

        updateFilters();
    }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        lowCutFilter.process(context);
        lowShelfFilter.process(context);
        highShelfFilter800.process(context);
        bellFilter800.process(context);
        highShelfFilter1410.process(context);
    }

    void updateLowShelf(float gainDb)
    {
        lowShelfGain = juce::Decibels::decibelsToGain(gainDb);
        updateLowShelfFilter();
    }

    void updateHighShelf800(float gainDb)
    {
        highShelf800Gain = juce::Decibels::decibelsToGain(gainDb);
        updateHighShelf800Filter();
    }

    void updateBellGain(float gainDb)
    {
        bell800Gain = juce::Decibels::decibelsToGain(gainDb);
        updateBell800Filter();
    }

    void updateHighShelf1410(float gainDb)
    {
        highShelf1410Gain = juce::Decibels::decibelsToGain(gainDb);
        updateHighShelf1410Filter();
    }

private:
    void updateFilters()
    {
        updateLowCutFilter();
        updateLowShelfFilter();
        updateHighShelf800Filter();
        updateBell800Filter();
        updateHighShelf1410Filter();
    }

    void updateLowCutFilter()
    {
        *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 19.4f, 0.24f);
    }

    void updateLowShelfFilter()
    {
        *lowShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 469.0f, 0.67f, lowShelfGain);
    }

    void updateHighShelf800Filter()
    {
        *highShelfFilter800.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 800.0f, 0.71f, highShelf800Gain);
    }

    void updateBell800Filter()
    {
        *bellFilter800.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 800.0f, 0.71f, bell800Gain);
    }

    void updateHighShelf1410Filter()
    {
        *highShelfFilter1410.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1410.0f, 0.70f, highShelf1410Gain);
    }

    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowCutFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowShelfFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highShelfFilter800;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bellFilter800;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highShelfFilter1410;

    float lowShelfGain = 1.0f;
    float highShelf800Gain = 1.0f;
    float bell800Gain = 1.0f;
    float highShelf1410Gain = 1.0f;
};
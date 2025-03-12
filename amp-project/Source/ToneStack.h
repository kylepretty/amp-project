#pragma once
#include <JuceHeader.h>
#include "AmpProfile.h"

class ToneStack
{
public:
    ToneStack() = default;

    /** Prepares the processor with the given sample rate and channel configuration. */
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

    /** Processes the audio block through all filters. */
    void process(juce::dsp::AudioBlock<float>& block)
    {
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        lowCutFilter.process(context);
        lowShelfFilter.process(context);
        highShelfFilter800.process(context);
        bellFilter800.process(context);
        highShelfFilter1410.process(context);
    }

    /** Sets the low shelf filter parameters from the AmpProfile. */
    void setLowShelfParams(float freq, float q, float gainDb)
    {
        lowShelfFreq = freq;
        lowShelfQ = q;
        lowShelfGain = juce::Decibels::decibelsToGain(gainDb);
    }

    /** Updates the low shelf gain, typically called by a slider. */
    void updateLowShelfGain(float gainDb)
    {
        lowShelfGain = juce::Decibels::decibelsToGain(gainDb);
        updateLowShelfFilter();
    }

    /** Sets the high shelf 800 Hz filter parameters from the AmpProfile. */
    void setHighShelf800Params(float freq, float q, float gainDb)
    {
        highShelf800Freq = freq;
        highShelf800Q = q;
        highShelf800Gain = juce::Decibels::decibelsToGain(gainDb);
    }

    /** Updates the high shelf 800 Hz gain, typically called by a slider. */
    void updateHighShelf800Gain(float gainDb)
    {
        highShelf800Gain = juce::Decibels::decibelsToGain(gainDb);
        updateHighShelf800Filter();
    }

    /** Sets the bell filter parameters from the AmpProfile. */
    void setBellParams(float freq, float q, float gainDb)
    {
        bellFreq = freq;
        bellQ = q;
        bellGain = juce::Decibels::decibelsToGain(gainDb);
    }

    /** Updates the bell filter gain, typically called by a slider. */
    void updateBellGain(float gainDb)
    {
        bellGain = juce::Decibels::decibelsToGain(gainDb);
        updateBell800Filter();
    }

    /** Sets the high shelf 1410 Hz filter parameters from the AmpProfile. */
    void setHighShelf1410Params(float freq, float q, float gainDb)
    {
        highShelf1410Freq = freq;
        highShelf1410Q = q;
        highShelf1410Gain = juce::Decibels::decibelsToGain(gainDb);
    }

    /** Updates the high shelf 1410 Hz gain, typically called by a slider. */
    void updateHighShelf1410Gain(float gainDb)
    {
        highShelf1410Gain = juce::Decibels::decibelsToGain(gainDb);
        updateHighShelf1410Filter();
    }

private:
    /** Updates all filter coefficients based on current parameters. */
    void updateFilters()
    {
        updateLowCutFilter();
        updateLowShelfFilter();
        updateHighShelf800Filter();
        updateBell800Filter();
        updateHighShelf1410Filter();
    }

    /** Updates the low cut filter coefficients (still hardcoded). */
    void updateLowCutFilter()
    {
        *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 19.4f, 0.24f);
    }

    /** Updates the low shelf filter coefficients using member variables. */
    void updateLowShelfFilter()
    {
        *lowShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowShelfFreq, lowShelfQ, lowShelfGain);
    }

    /** Updates the high shelf 800 Hz filter coefficients using member variables. */
    void updateHighShelf800Filter()
    {
        *highShelfFilter800.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highShelf800Freq, highShelf800Q, highShelf800Gain);
    }

    /** Updates the bell filter coefficients using member variables. */
    void updateBell800Filter()
    {
        *bellFilter800.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, bellFreq, bellQ, bellGain);
    }

    /** Updates the high shelf 1410 Hz filter coefficients using member variables. */
    void updateHighShelf1410Filter()
    {
        *highShelfFilter1410.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highShelf1410Freq, highShelf1410Q, highShelf1410Gain);
    }

    // Sample rate for filter calculations
    double sampleRate = 44100.0;

    // Filter processors
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowCutFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowShelfFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highShelfFilter800;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bellFilter800;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highShelfFilter1410;

    // Low shelf parameters
    float lowShelfFreq = 469.0f;
    float lowShelfQ = 0.67f;
    float lowShelfGain = 1.0f;

    // High shelf 800 Hz parameters
    float highShelf800Freq = 800.0f;
    float highShelf800Q = 0.71f;
    float highShelf800Gain = 1.0f;

    // Bell filter parameters
    float bellFreq = 800.0f;
    float bellQ = 0.71f;
    float bellGain = 1.0f;

    // High shelf 1410 Hz parameters
    float highShelf1410Freq = 1410.0f;
    float highShelf1410Q = 0.70f;
    float highShelf1410Gain = 1.0f;
};
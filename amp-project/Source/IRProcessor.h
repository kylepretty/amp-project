#pragma once
#include <JuceHeader.h>
#include "AmpProfile.h"

class IRProcessor
{
public:
    IRProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        convolutionCabinet.prepare(spec);
        convolutionReverb.prepare(spec);
        cabinetWetBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true, true);
        reverbWetBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true, true);
        dryBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true, true);
        cabinetMixSmoothed.reset(spec.sampleRate, 0.005f);
        reverbGainSmoothed.reset(spec.sampleRate, 0.005f);
        cabinetGainSmoothed.reset(spec.sampleRate, 0.005f);
    }

    bool loadReverbIR(const juce::File& file)
    {
        if (!file.existsAsFile()) return false;

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr) return false;

        // Load the reverb IR
        juce::AudioBuffer<float> irBuffer;
        irBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
        reader->read(&irBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

        // Add 10 ms predelay
        float predelayMs = 10.0f;
        int predelaySamples = static_cast<int>(std::round(predelayMs * sampleRate / 1000.0));
        juce::AudioBuffer<float> irWithPredelay(irBuffer.getNumChannels(), predelaySamples + irBuffer.getNumSamples());
        for (int ch = 0; ch < irWithPredelay.getNumChannels(); ++ch)
        {
            irWithPredelay.clear(ch, 0, predelaySamples); // Prepend silence
            irWithPredelay.copyFrom(ch, predelaySamples, irBuffer, ch, 0, irBuffer.getNumSamples());
        }

        // Load the modified reverb IR into the convolution
        convolutionReverb.loadImpulseResponse(std::move(irWithPredelay), sampleRate,
                                              juce::dsp::Convolution::Stereo::yes,
                                              juce::dsp::Convolution::Trim::yes, 
                                              juce::dsp::Convolution::Normalise::yes);
        return true;
    }

    // Load the cabinet IR
    bool loadCabinetIR(const juce::File& file)
    {
        if (!file.existsAsFile()) return false;
        convolutionCabinet.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::yes,
                                               juce::dsp::Convolution::Trim::yes, 0);
        return true;
    }

    void process(juce::dsp::AudioBlock<float>& block, bool useMix)
    {
        float cabinetMix = cabinetMixSmoothed.getNextValue();
        float reverbGain = juce::Decibels::decibelsToGain(reverbGainSmoothed.getNextValue());
        float cabinetGain = juce::Decibels::decibelsToGain(cabinetGainSmoothed.getNextValue());

        // Dry signal
        juce::dsp::AudioBlock<float> dryBlock = juce::dsp::AudioBlock<float>(dryBuffer).getSubBlock(0, block.getNumSamples());
        dryBlock.copyFrom(block);

        // Reverb signal
        juce::dsp::AudioBlock<float> reverbWetBlock = juce::dsp::AudioBlock<float>(reverbWetBuffer).getSubBlock(0, block.getNumSamples());
        reverbWetBlock.copyFrom(dryBlock);
        juce::dsp::ProcessContextReplacing<float> reverbContext(reverbWetBlock);
        if (reverbGain > 0.0f)
            convolutionReverb.process(reverbContext);

        // Mix reverb on to dry signal
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto* outData = block.getChannelPointer(channel);
            auto* dryData = dryBlock.getChannelPointer(channel);
            auto* revWetData = reverbWetBlock.getChannelPointer(channel);
            juce::FloatVectorOperations::copy(outData, dryData, block.getNumSamples());
            juce::FloatVectorOperations::addWithMultiply(outData, revWetData, reverbGain, block.getNumSamples());
        }

        // Cabinet signal
        juce::dsp::AudioBlock<float> cabinetWetBlock = juce::dsp::AudioBlock<float>(cabinetWetBuffer).getSubBlock(0, block.getNumSamples());
        cabinetWetBlock.copyFrom(block);
        juce::dsp::ProcessContextReplacing<float> cabinetContext(cabinetWetBlock);
        if (cabinetMix > 0.0f || (!useMix && cabinetGain > 0.0f))
            convolutionCabinet.process(cabinetContext);

        // Mix cabinet signal on to mixed reverb
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto* outData = block.getChannelPointer(channel);
            auto* reverbOutData = block.getChannelPointer(channel);
            auto* cabWetData = cabinetWetBlock.getChannelPointer(channel);
            if (useMix)
            {
                // Wet/dry mix for cabinet
                juce::FloatVectorOperations::multiply(cabWetData, cabinetMix, block.getNumSamples());
                juce::FloatVectorOperations::multiply(reverbOutData, 1.0f - cabinetMix, block.getNumSamples());
                juce::FloatVectorOperations::add(outData, reverbOutData, cabWetData, block.getNumSamples());
            }
            else
            {
                // Gain for cabinet
                juce::FloatVectorOperations::copy(outData, reverbOutData, block.getNumSamples());
                juce::FloatVectorOperations::addWithMultiply(outData, cabWetData, cabinetGain, block.getNumSamples());
            }
        }
    }
    void setCabinetMix(float newValue) { cabinetMixSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, newValue)); }
    void setReverbGain(float newValue) { reverbGainSmoothed.setTargetValue(juce::jlimit(0.0f, 24.0f, newValue)); }
    void setCabinetGain(float newValue) { cabinetGainSmoothed.setTargetValue(juce::jlimit(-12.0f, 12.0f, newValue)); }

private:
    juce::dsp::Convolution convolutionCabinet;
    juce::dsp::Convolution convolutionReverb;
    juce::AudioBuffer<float> cabinetWetBuffer;
    juce::AudioBuffer<float> reverbWetBuffer;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> cabinetMixSmoothed{1.0f};
    juce::SmoothedValue<float> cabinetGainSmoothed{0.0f};
    juce::SmoothedValue<float> reverbGainSmoothed{0.0f};
    double sampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRProcessor)
};
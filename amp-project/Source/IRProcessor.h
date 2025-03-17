#pragma once
#include <JuceHeader.h>

class IRProcessor
{
public:
    IRProcessor()
    {
        dryDelayLine.setMaximumDelayInSamples(8192);
        cabinetBypass = true;
        reverbBypass = true;
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        convolutionCabinet.prepare(spec);
        convolutionReverb.prepare(spec);

        cabinetWetBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true, false);
        reverbWetBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true, false);
        dryBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true, false);

        dryDelayLine.prepare({ spec.sampleRate, spec.maximumBlockSize, spec.numChannels });
        updateLatencyCompensation();

        reverbGainSmoothed.reset(spec.sampleRate, 0.001f);
    }

    bool loadCabinetIR(const juce::File& file)
    {
        if (!file.existsAsFile())
        {
            juce::Logger::writeToLog("Cabinet IR file not found: " + file.getFullPathName());
            return false;
        }
        convolutionCabinet.loadImpulseResponse(
            file,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::yes,
            0,
            juce::dsp::Convolution::Normalise::yes
        );
        cabinetBypass = false;
        updateLatencyCompensation();
        juce::Logger::writeToLog("Cabinet IR loaded and normalized: " + file.getFullPathName());
        return true;
    }

    bool loadReverbIR(const juce::File& file)
    {
        if (!file.existsAsFile())
        {
            juce::Logger::writeToLog("Reverb IR file not found: " + file.getFullPathName());
            return false;
        }
        convolutionReverb.loadImpulseResponse(
            file,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::yes,
            0,
            juce::dsp::Convolution::Normalise::yes
        );
        reverbBypass = false;
        updateLatencyCompensation();
        juce::Logger::writeToLog("Reverb IR loaded and normalized: " + file.getFullPathName());
        return true;
    }

    void resetCabinetIR()
    {
        cabinetBypass = true;
        updateLatencyCompensation();
    }

    void resetReverbIR()
    {
        reverbBypass = true;
        updateLatencyCompensation();
    }

    void process(juce::dsp::AudioBlock<float>& block, bool useMix)
    {
        auto numSamples = block.getNumSamples();
        auto numChannels = block.getNumChannels();

        jassert(cabinetWetBuffer.getNumSamples() >= numSamples);
        jassert(reverbWetBuffer.getNumSamples() >= numSamples);
        jassert(dryBuffer.getNumSamples() >= numSamples);

        float reverbGain = juce::Decibels::decibelsToGain(reverbGainSmoothed.getNextValue());

        juce::dsp::AudioBlock<float> dryBlock(dryBuffer);
        dryBlock.copyFrom(block);
        juce::dsp::ProcessContextReplacing<float> dryContext(dryBlock);
        dryDelayLine.process(dryContext);

        juce::dsp::AudioBlock<float> reverbWetBlock(reverbWetBuffer);
        reverbWetBlock.copyFrom(block);
        if (!reverbBypass)
        {
            juce::dsp::ProcessContextReplacing<float> reverbContext(reverbWetBlock);
            convolutionReverb.process(reverbContext);
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* dryData = dryBlock.getChannelPointer(channel);
            auto* revWetData = reverbWetBlock.getChannelPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                revWetData[sample] = dryData[sample] + (reverbGain > 0.0f && !reverbBypass ? reverbGain * revWetData[sample] : 0.0f);
            }
        }

        juce::dsp::AudioBlock<float> cabinetWetBlock(cabinetWetBuffer);
        cabinetWetBlock.copyFrom(reverbWetBlock);
        if (!cabinetBypass)
        {
            juce::dsp::ProcessContextReplacing<float> cabinetContext(cabinetWetBlock);
            convolutionCabinet.process(cabinetContext);
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* cabWetData = cabinetWetBlock.getChannelPointer(channel);
            auto* outData = block.getChannelPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                outData[sample] = cabWetData[sample];
            }
        }
    }

    void setReverbGain(float newValue) { reverbGainSmoothed.setTargetValue(juce::jlimit(-12.0f, 12.0f, newValue)); }

private:
    void updateLatencyCompensation()
    {
        int totalLatency = (cabinetBypass ? 0 : convolutionCabinet.getLatency()) +
            (reverbBypass ? 0 : convolutionReverb.getLatency());
        dryDelayLine.setDelay(static_cast<float>(totalLatency));
    }

    juce::dsp::Convolution convolutionCabinet;
    juce::dsp::Convolution convolutionReverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> dryDelayLine;
    juce::AudioBuffer<float> cabinetWetBuffer;
    juce::AudioBuffer<float> reverbWetBuffer;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> cabinetMixSmoothed{ 1.0f };
    juce::SmoothedValue<float> cabinetGainSmoothed{ 0.0f };
    juce::SmoothedValue<float> reverbGainSmoothed{ 0.0f };
    bool cabinetBypass;
    bool reverbBypass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRProcessor)
};
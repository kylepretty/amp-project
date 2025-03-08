#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"
#include "ToneStack.h"

class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent()
    {
        setSize(1280, 720);

        if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
            && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
        {
            juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                [this](bool granted) { setAudioChannels(granted ? 1 : 0, 2); });
        }
        else
        {
            setAudioChannels(1, 2);
        }

        auto shaperFunction = [](float sample) {
            return sample / (std::abs(sample) + 1.0f);
            };

        waveShaperPreEQ.functionToUse = shaperFunction;
        waveShaperPostEQ.functionToUse = shaperFunction;

        addAndMakeVisible(openMenu);
        openMenu.onClick = [this]() { openIOMenuWindow(); };

        addAndMakeVisible(inputGainSlider);
        inputGainSlider.setRange(0.0, 2.0, 0.01);
        inputGainSlider.setValue(1.0);
        inputGainSlider.onValueChange = [this]() { gain = inputGainSlider.getValue(); };

        addAndMakeVisible(outputGainSlider);
        outputGainSlider.setRange(0.0, 4.0, 0.01);
        outputGainSlider.setValue(1.0);
        outputGainSlider.onValueChange = [this]() { outputGain = outputGainSlider.getValue(); };

        addAndMakeVisible(lowShelfGainSlider);
        lowShelfGainSlider.setRange(-10.0, 3.5, 0.1);
        lowShelfGainSlider.setValue(0.0);
        lowShelfGainSlider.onValueChange = [this]() { eq.updateLowShelf(lowShelfGainSlider.getValue()); };

        addAndMakeVisible(highShelf800GainSlider);
        highShelf800GainSlider.setRange(-4.0, 0.0, 0.1);
        highShelf800GainSlider.setValue(0.0);
        highShelf800GainSlider.onValueChange = [this]() { eq.updateHighShelf800(highShelf800GainSlider.getValue()); };

        addAndMakeVisible(bellGainSlider);
        bellGainSlider.setRange(-17.0, -8.0, 0.1);
        bellGainSlider.setValue(-8.0);
        bellGainSlider.onValueChange = [this]() { eq.updateBellGain(bellGainSlider.getValue()); };

        addAndMakeVisible(highShelf1410GainSlider);
        highShelf1410GainSlider.setRange(-10.0, 0.0, 0.1);
        highShelf1410GainSlider.setValue(0.0);
        highShelf1410GainSlider.onValueChange = [this]() { eq.updateHighShelf1410(highShelf1410GainSlider.getValue()); };
    }

    ~MainComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlockExpected;
        spec.numChannels = 2;

        waveShaperPreEQ.prepare(spec);
        waveShaperPostEQ.prepare(spec);
        eq.prepare(spec);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        juce::dsp::AudioBlock<float> block(*bufferToFill.buffer);

        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            juce::FloatVectorOperations::multiply(block.getChannelPointer(channel),
                gain,
                bufferToFill.numSamples);
        }

        auto* leftChannel = block.getChannelPointer(0);
        auto* rightChannel = block.getChannelPointer(1);
        for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
            float processedSample = waveShaperPreEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }

        eq.process(block);

        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            juce::FloatVectorOperations::multiply(block.getChannelPointer(channel),
                outputGain,
                bufferToFill.numSamples);
        }

        for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
            float processedSample = waveShaperPostEQ.processSample(leftChannel[sample]);
            leftChannel[sample] = processedSample;
            rightChannel[sample] = processedSample;
        }

        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            auto* channelData = block.getChannelPointer(channel);
            for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
                channelData[sample] = juce::jlimit(-1.0f, 1.0f, channelData[sample]);
            }
        }
    }

    void releaseResources() override {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        openMenu.setBounds(10, 10, 150, 40);
        inputGainSlider.setBounds(10, 60, 200, 40);
        outputGainSlider.setBounds(10, 110, 200, 40);
        lowShelfGainSlider.setBounds(10, 160, 200, 40);
        highShelf800GainSlider.setBounds(10, 210, 200, 40);
        bellGainSlider.setBounds(10, 260, 200, 40);
        highShelf1410GainSlider.setBounds(10, 310, 200, 40);
    }

private:
    juce::TextButton openMenu{ "I/O Menu", "Open I/O Menu" };
    juce::Component::SafePointer<IOMenuWindow> ioMenuWindow;
    juce::dsp::WaveShaper<float> waveShaperPreEQ;
    juce::dsp::WaveShaper<float> waveShaperPostEQ;
    ToneStack eq;

    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Slider lowShelfGainSlider;
    juce::Slider highShelf800GainSlider;
    juce::Slider bellGainSlider;
    juce::Slider highShelf1410GainSlider;

    float gain = 1.0f;
    float outputGain = 1.0f;

    void openIOMenuWindow()
    {
        if (ioMenuWindow == nullptr)
        {
            auto* window = new IOMenuWindow("I/O Menu", deviceManager);
            ioMenuWindow = window;
            window->setVisible(true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
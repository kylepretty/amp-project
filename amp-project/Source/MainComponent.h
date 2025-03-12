#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"
#include "ToneStack.h"
#include "WaveshaperProcessor.h"
#include "IRProcessor.h"
#include "AmpProfile.h"

// Waveshaper functions
inline float waveshaperClean(float x) { return std::tanh(x); }
inline float waveshaperCrunch(float x) { return x / (1.0f + std::abs(x)); }
inline float waveshaperLead(float x) { return x * (std::abs(x) + 0.5f) / (x * x + 0.25f); }

class MainComponent : public juce::AudioAppComponent, private juce::Button::Listener
{
public:
    MainComponent()
        : profile1{
            waveshaperClean, waveshaperClean,
            0.0f, 2.0f, 1.0f,
            {100.0f, 0.707f, -10.0f, 3.5f, 0.0f},
            {800.0f, 0.707f, -4.0f, 0.0f, 0.0f},
            {1200.0f, 1.0f, -17.0f, -8.0f, -8.0f},
            {1410.0f, 0.707f, -10.0f, 0.0f, 0.0f},
            "C:/Users/kylep/source/repos/amp-project/amp-project/Source/reverb_clean.wav",
            "C:/Users/kylep/source/repos/amp-project/amp-project/Source/cabinet_4x12_clean.wav",
            0.0f, 1.0f, 1.0f,
            -12.0f, 12.0f, 0.0f,
            0.0f, 12.0f, 0.0f
        },
        profile2{
            waveshaperCrunch, waveshaperCrunch,
            0.0f, 3.0f, 1.5f,
            {80.0f, 1.0f, -12.0f, 6.0f, 3.0f},
            {800.0f, 1.0f, -6.0f, 2.0f, -2.0f},
            {1000.0f, 1.5f, -20.0f, -6.0f, -10.0f},
            {1410.0f, 1.0f, -12.0f, 2.0f, 0.0f},
            "C:/Users/kylep/source/repos/amp-project/amp-project/Source/reverb_crunch.wav",
            "C:/Users/kylep/source/repos/amp-project/amp-project/Source/cabinet_2x12_crunch.wav",
            0.0f, 1.0f, 0.8f,
            -12.0f, 12.0f, 2.0f,
            0.0f, 18.0f, 6.0f
        },
        profile3{
            waveshaperLead, waveshaperLead,
            0.0f, 4.0f, 2.0f,
            {120.0f, 0.9f, -15.0f, 9.0f, 4.0f},
            {800.0f, 0.8f, -8.0f, 4.0f, 0.0f},
            {1400.0f, 2.0f, -22.0f, -4.0f, -12.0f},
            {1410.0f, 0.9f, -15.0f, 4.0f, 2.0f},
            "C:/Users/kylep/source/repos/amp-project/amp-project/Source/reverb_lead.wav",
            "C:/Users/kylep/source/repos/amp-project/amp-project/Source/cabinet_1x12_lead.wav",
            0.0f, 1.0f, 0.9f,
            -12.0f, 12.0f, 4.0f,
            0.0f, 24.0f, 8.0f
        }
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

        // UI Setup
        addAndMakeVisible(openMenu);
        openMenu.onClick = [this]() { openIOMenuWindow(); };

        addAndMakeVisible(inputGainSlider);
        inputGainSlider.setRange(0.0, 2.0, 0.01);
        inputGainSlider.setValue(1.0);
        inputGainSlider.setSliderStyle(juce::Slider::Rotary);
        inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        inputGainSlider.onValueChange = [this]() { gain = inputGainSlider.getValue(); };
        addAndMakeVisible(inputGainLabel);
        inputGainLabel.setText("Input Gain", juce::dontSendNotification);
        inputGainLabel.attachToComponent(&inputGainSlider, true);

        addAndMakeVisible(outputGainSlider);
        outputGainSlider.setRange(0.0, 4.0, 0.01);
        outputGainSlider.setValue(1.0);
        outputGainSlider.setSliderStyle(juce::Slider::Rotary);
        outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        outputGainSlider.onValueChange = [this]() { outputGain = outputGainSlider.getValue(); };
        addAndMakeVisible(outputGainLabel);
        outputGainLabel.setText("Output Gain", juce::dontSendNotification);
        outputGainLabel.attachToComponent(&outputGainSlider, true);

        // EQ Sliders
        addAndMakeVisible(lowShelfGainSlider);
        lowShelfGainSlider.setRange(-10.0, 3.5, 0.1);
        lowShelfGainSlider.setValue(0.0);
        lowShelfGainSlider.setSliderStyle(juce::Slider::Rotary);
        lowShelfGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        lowShelfGainSlider.onValueChange = [this]() { eq.updateLowShelfGain(lowShelfGainSlider.getValue()); };
        addAndMakeVisible(lowShelfGainLabel);
        lowShelfGainLabel.setText("Low Shelf Gain (dB)", juce::dontSendNotification);
        lowShelfGainLabel.attachToComponent(&lowShelfGainSlider, true);

        addAndMakeVisible(highShelf800GainSlider);
        highShelf800GainSlider.setRange(-4.0, 0.0, 0.1);
        highShelf800GainSlider.setValue(0.0);
        highShelf800GainSlider.setSliderStyle(juce::Slider::Rotary);
        highShelf800GainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        highShelf800GainSlider.onValueChange = [this]() { eq.updateHighShelf800Gain(highShelf800GainSlider.getValue()); };
        addAndMakeVisible(highShelf800GainLabel);
        highShelf800GainLabel.setText("High Shelf 800 Hz (dB)", juce::dontSendNotification);
        highShelf800GainLabel.attachToComponent(&highShelf800GainSlider, true);

        addAndMakeVisible(bellGainSlider);
        bellGainSlider.setRange(-17.0, -8.0, 0.1);
        bellGainSlider.setValue(-8.0);
        bellGainSlider.setSliderStyle(juce::Slider::Rotary);
        bellGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        bellGainSlider.onValueChange = [this]() { eq.updateBellGain(bellGainSlider.getValue()); };
        addAndMakeVisible(bellGainLabel);
        bellGainLabel.setText("Bell Gain (dB)", juce::dontSendNotification);
        bellGainLabel.attachToComponent(&bellGainSlider, true);

        addAndMakeVisible(highShelf1410GainSlider);
        highShelf1410GainSlider.setRange(-10.0, 0.0, 0.1);
        highShelf1410GainSlider.setValue(0.0);
        highShelf1410GainSlider.setSliderStyle(juce::Slider::Rotary);
        highShelf1410GainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        highShelf1410GainSlider.onValueChange = [this]() { eq.updateHighShelf1410Gain(highShelf1410GainSlider.getValue()); };
        addAndMakeVisible(highShelf1410GainLabel);
        highShelf1410GainLabel.setText("High Shelf 1410 Hz (dB)", juce::dontSendNotification);
        highShelf1410GainLabel.attachToComponent(&highShelf1410GainSlider, true);

        // Cabinet Mix Slider
        addAndMakeVisible(cabinetMixSlider);
        cabinetMixSlider.setRange(0.0, 1.0, 0.01);
        cabinetMixSlider.setValue(1.0);
        cabinetMixSlider.setSliderStyle(juce::Slider::Rotary);
        cabinetMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        cabinetMixSlider.onValueChange = [this]() { irProcessor.setCabinetMix(cabinetMixSlider.getValue()); };
        addAndMakeVisible(cabinetMixLabel);
        cabinetMixLabel.setText("Cabinet Wet/Dry Mix", juce::dontSendNotification);
        cabinetMixLabel.attachToComponent(&cabinetMixSlider, true);

        // Cabinet Gain Slider
        addAndMakeVisible(cabinetGainSlider);
        cabinetGainSlider.setRange(-12.0, 12.0, 0.1);
        cabinetGainSlider.setValue(0.0);
        cabinetGainSlider.setSliderStyle(juce::Slider::Rotary);
        cabinetMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        cabinetGainSlider.onValueChange = [this]() { irProcessor.setCabinetGain(cabinetGainSlider.getValue()); };
        addAndMakeVisible(cabinetGainLabel);
        cabinetGainLabel.setText("Cabinet Gain (dB)", juce::dontSendNotification);
        cabinetGainLabel.attachToComponent(&cabinetGainSlider, true);

        // Reverb Gain Slider
        addAndMakeVisible(reverbGainSlider);
        reverbGainSlider.setRange(-12.0, 12.0, 0.1);
        reverbGainSlider.setValue(0.0);
        reverbGainSlider.setSliderStyle(juce::Slider::Rotary);
        reverbGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        reverbGainSlider.onValueChange = [this]() { irProcessor.setReverbGain(reverbGainSlider.getValue()); };
        addAndMakeVisible(reverbGainLabel);
        reverbGainLabel.setText("Reverb Gain (dB)", juce::dontSendNotification);
        reverbGainLabel.attachToComponent(&reverbGainSlider, true);

        // Profile Buttons
        addAndMakeVisible(profileButton1);
        addAndMakeVisible(profileButton2);
        addAndMakeVisible(profileButton3);
        profileButton1.setButtonText("Clean");
        profileButton2.setButtonText("Crunch");
        profileButton3.setButtonText("Lead");
        profileButton1.addListener(this);
        profileButton2.addListener(this);
        profileButton3.addListener(this);

        // Load initial profile
        loadProfile(profile1);
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

        waveshaper.prepare(spec);
        eq.prepare(spec);
        irProcessor.prepare(spec);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        juce::dsp::AudioBlock<float> block(*bufferToFill.buffer);

        // Apply input gain
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            juce::FloatVectorOperations::multiply(block.getChannelPointer(channel),
                gain, bufferToFill.numSamples);
        }

        // Process audio chain
        waveshaper.processPreEQ(block);
        eq.process(block);
        waveshaper.processPostEQ(block);
        irProcessor.process(block, true);

        // Apply output gain and limit
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            juce::FloatVectorOperations::multiply(block.getChannelPointer(channel),
                outputGain, bufferToFill.numSamples);
            auto* channelData = block.getChannelPointer(channel);
            for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
                channelData[sample] = juce::jlimit(-1.0f, 1.0f, channelData[sample]);
            }
        }
    }

    void releaseResources() override
    {
        // No explicit resources to release in this case, but required to be implemented
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        int labelWidth = 150;
        int sliderWidth = 200;
        int sliderHeight = 100;
        int labelHeight = 10;

        profileButton1.setBounds(10, labelHeight, 100, 30);
        profileButton2.setBounds(120, labelHeight, 100, 30);
        profileButton3.setBounds(230, labelHeight, 100, 30);
        labelHeight += 40;

        openMenu.setBounds(10, labelHeight, 150, 40);
        labelHeight += 440;

        inputGainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelHeight += 100;
        outputGainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelWidth += 300;
        labelHeight -= 100;
        lowShelfGainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelHeight += 100;
        highShelf800GainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelWidth += 300;
        labelHeight -= 100;
        bellGainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelHeight += 100;
        highShelf1410GainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelWidth += 300;
        labelHeight -= 200;
        cabinetMixSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelHeight += 100;
        cabinetGainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
        labelHeight += 100;
        reverbGainSlider.setBounds(labelWidth, labelHeight, sliderWidth, sliderHeight);
    }

private:
    // UI Components
    juce::TextButton openMenu{ "I/O Menu", "Open I/O Menu" };
    juce::Component::SafePointer<IOMenuWindow> ioMenuWindow;
    juce::TextButton profileButton1, profileButton2, profileButton3;

    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    juce::Slider lowShelfGainSlider;
    juce::Label lowShelfGainLabel;
    juce::Slider highShelf800GainSlider;
    juce::Label highShelf800GainLabel;
    juce::Slider bellGainSlider;
    juce::Label bellGainLabel;
    juce::Slider highShelf1410GainSlider;
    juce::Label highShelf1410GainLabel;
    juce::Slider cabinetMixSlider;
    juce::Label cabinetMixLabel;
    juce::Slider cabinetGainSlider;
    juce::Label cabinetGainLabel;
    juce::Slider reverbGainSlider;
    juce::Label reverbGainLabel;

    // Processors
    WaveshaperProcessor waveshaper;
    ToneStack eq;
    IRProcessor irProcessor;

    // Profiles
    AmpProfile profile1, profile2, profile3;

    // Variables
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

    void buttonClicked(juce::Button* button) override
    {
        if (button == &profileButton1) loadProfile(profile1);
        else if (button == &profileButton2) loadProfile(profile2);
        else if (button == &profileButton3) loadProfile(profile3);
    }

    void loadProfile(const AmpProfile& profile)
    {
        waveshaper.setPreEQWaveshaperFunction(profile.preEQWaveshaperFunction);
        waveshaper.setPostEQWaveshaperFunction(profile.postEQWaveshaperFunction);

        inputGainSlider.setRange(profile.gainMin, profile.gainMax, 0.01);
        inputGainSlider.setValue(profile.gainDefault, juce::dontSendNotification);
        gain = profile.gainDefault;

        eq.setLowShelfParams(profile.lowShelf.freq, profile.lowShelf.q, profile.lowShelf.defaultGain);
        lowShelfGainSlider.setRange(profile.lowShelf.minGain, profile.lowShelf.maxGain, 0.1);
        lowShelfGainSlider.setValue(profile.lowShelf.defaultGain, juce::dontSendNotification);

        eq.setHighShelf800Params(profile.highShelf800.freq, profile.highShelf800.q, profile.highShelf800.defaultGain);
        highShelf800GainSlider.setRange(profile.highShelf800.minGain, profile.highShelf800.maxGain, 0.1);
        highShelf800GainSlider.setValue(profile.highShelf800.defaultGain, juce::dontSendNotification);

        eq.setBellParams(profile.bell.freq, profile.bell.q, profile.bell.defaultGain);
        bellGainSlider.setRange(profile.bell.minGain, profile.bell.maxGain, 0.1);
        bellGainSlider.setValue(profile.bell.defaultGain, juce::dontSendNotification);

        eq.setHighShelf1410Params(profile.highShelf1410.freq, profile.highShelf1410.q, profile.highShelf1410.defaultGain);
        highShelf1410GainSlider.setRange(profile.highShelf1410.minGain, profile.highShelf1410.maxGain, 0.1);
        highShelf1410GainSlider.setValue(profile.highShelf1410.defaultGain, juce::dontSendNotification);

        irProcessor.loadReverbIR(juce::File(profile.reverbIRPath));
        irProcessor.loadCabinetIR(juce::File(profile.cabinetIRPath));

        cabinetMixSlider.setRange(profile.cabinetMixMin, profile.cabinetMixMax, 0.01);
        cabinetMixSlider.setValue(profile.cabinetMixDefault, juce::dontSendNotification);
        irProcessor.setCabinetMix(profile.cabinetMixDefault);

        cabinetGainSlider.setRange(profile.cabinetGainMin, profile.cabinetGainMax, 0.1);
        cabinetGainSlider.setValue(profile.cabinetGainDefault, juce::dontSendNotification);
        irProcessor.setCabinetGain(profile.cabinetGainDefault);

        reverbGainSlider.setRange(profile.reverbGainMin, profile.reverbGainMax, 0.1);
        reverbGainSlider.setValue(profile.reverbGainDefault, juce::dontSendNotification);
        irProcessor.setReverbGain(profile.reverbGainDefault);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
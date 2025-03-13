#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"
#include "ToneStack.h"
#include "WaveshaperProcessor.h"
#include "IRProcessor.h"

class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent()
    {
        setSize(1280, 720);

        // Cabinet IR selection ComboBox
        addAndMakeVisible(cabinetIrSelector);
        cabinetIrSelector.addItem("Select Cabinet IR", 1);
        cabinetIrSelector.setSelectedId(1);
        loadCabinetIRsFromFolder();
        cabinetIrSelector.onChange = [this]() {
            if (cabinetIrSelector.getSelectedId() == 1) {
                irProcessor.resetCabinetIR(); // Reset when default is selected
                juce::Logger::writeToLog("Cabinet IR reset to default (no IR)");
            }
            else if (cabinetIrSelector.getSelectedId() > 1) {
                juce::File selectedFile = cabinetIrFiles[cabinetIrSelector.getSelectedId() - 2];
                if (!irProcessor.loadCabinetIR(selectedFile)) {
                    juce::Logger::writeToLog("Failed to load selected cabinet IR: " + selectedFile.getFullPathName());
                }
            }
            };

        // Reverb IR selection ComboBox
        addAndMakeVisible(reverbIrSelector);
        reverbIrSelector.addItem("Select Reverb IR", 1);
        reverbIrSelector.setSelectedId(1);
        loadReverbIRsFromFolder();
        reverbIrSelector.onChange = [this]() {
            if (reverbIrSelector.getSelectedId() == 1) {
                irProcessor.resetReverbIR(); // Reset when default is selected
                juce::Logger::writeToLog("Reverb IR reset to default (no IR)");
            }
            else if (reverbIrSelector.getSelectedId() > 1) {
                juce::File selectedFile = reverbIrFiles[reverbIrSelector.getSelectedId() - 2];
                if (!irProcessor.loadReverbIR(selectedFile)) {
                    juce::Logger::writeToLog("Failed to load selected reverb IR: " + selectedFile.getFullPathName());
                }
            }
            };

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

        // UI Setup (unchanged)
        addAndMakeVisible(openMenu);
        openMenu.onClick = [this]() { openIOMenuWindow(); };

        addAndMakeVisible(inputGainSlider);
        inputGainSlider.setRange(0.0, 2.0, 0.01);
        inputGainSlider.setValue(1.0);
        inputGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        inputGainSlider.onValueChange = [this]() { gain = inputGainSlider.getValue(); };
        addAndMakeVisible(inputGainLabel);
        inputGainLabel.setText("Input Gain", juce::dontSendNotification);
        inputGainLabel.attachToComponent(&inputGainSlider, true);

        addAndMakeVisible(outputGainSlider);
        outputGainSlider.setRange(0.0, 4.0, 0.01);
        outputGainSlider.setValue(1.0);
        outputGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        outputGainSlider.onValueChange = [this]() { outputGain = outputGainSlider.getValue(); };
        addAndMakeVisible(outputGainLabel);
        outputGainLabel.setText("Output Gain", juce::dontSendNotification);
        outputGainLabel.attachToComponent(&outputGainSlider, true);

        // EQ Sliders (unchanged)
        addAndMakeVisible(lowShelfGainSlider);
        lowShelfGainSlider.setRange(-10.0, 3.5, 0.1);
        lowShelfGainSlider.setValue(0.0);
        lowShelfGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        lowShelfGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        lowShelfGainSlider.onValueChange = [this]() { eq.updateLowShelf(lowShelfGainSlider.getValue()); };
        addAndMakeVisible(lowShelfGainLabel);
        lowShelfGainLabel.setText("Low Shelf Gain (dB)", juce::dontSendNotification);
        lowShelfGainLabel.attachToComponent(&lowShelfGainSlider, true);

        addAndMakeVisible(highShelf800GainSlider);
        highShelf800GainSlider.setRange(-4.0, 0.0, 0.1);
        highShelf800GainSlider.setValue(0.0);
        highShelf800GainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        highShelf800GainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        highShelf800GainSlider.onValueChange = [this]() { eq.updateHighShelf800(highShelf800GainSlider.getValue()); };
        addAndMakeVisible(highShelf800GainLabel);
        highShelf800GainLabel.setText("High Shelf 800 Hz (dB)", juce::dontSendNotification);
        highShelf800GainLabel.attachToComponent(&highShelf800GainSlider, true);

        addAndMakeVisible(bellGainSlider);
        bellGainSlider.setRange(-17.0, -8.0, 0.1);
        bellGainSlider.setValue(-8.0);
        bellGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        bellGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        bellGainSlider.onValueChange = [this]() { eq.updateBellGain(bellGainSlider.getValue()); };
        addAndMakeVisible(bellGainLabel);
        bellGainLabel.setText("Bell Gain (dB)", juce::dontSendNotification);
        bellGainLabel.attachToComponent(&bellGainSlider, true);

        addAndMakeVisible(highShelf1410GainSlider);
        highShelf1410GainSlider.setRange(-10.0, 0.0, 0.1);
        highShelf1410GainSlider.setValue(0.0);
        highShelf1410GainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        highShelf1410GainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        highShelf1410GainSlider.onValueChange = [this]() { eq.updateHighShelf1410(highShelf1410GainSlider.getValue()); };
        addAndMakeVisible(highShelf1410GainLabel);
        highShelf1410GainLabel.setText("High Shelf 1410 Hz (dB)", juce::dontSendNotification);
        highShelf1410GainLabel.attachToComponent(&highShelf1410GainSlider, true);

        // Reverb Gain Slider
        addAndMakeVisible(reverbGainSlider);
        reverbGainSlider.setRange(-12.0, 12.0, 0.1);
        reverbGainSlider.setValue(0.0);
        reverbGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        reverbGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        reverbGainSlider.onValueChange = [this]() { irProcessor.setReverbGain(reverbGainSlider.getValue()); };
        addAndMakeVisible(reverbGainLabel);
        reverbGainLabel.setText("Reverb Gain (dB)", juce::dontSendNotification);
        reverbGainLabel.attachToComponent(&reverbGainSlider, true);
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

        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            juce::FloatVectorOperations::multiply(block.getChannelPointer(channel),
                gain,
                bufferToFill.numSamples);
        }

        waveshaper.processPreEQ(block);
        eq.process(block);
        waveshaper.processPostEQ(block);
        irProcessor.process(block, true);

        for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
            juce::FloatVectorOperations::multiply(block.getChannelPointer(channel),
                outputGain,
                bufferToFill.numSamples);
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
        int labelWidth = 150;
        int sliderWidth = 200;
        int y = 10;

        // Place both selectors at the top
        cabinetIrSelector.setBounds(10, y, 300, 30);
        reverbIrSelector.setBounds(320, y, 300, 30); // Positioned next to cabinet selector
        y += 40;

        openMenu.setBounds(10, y, 150, 40);
        y += 50;
        inputGainSlider.setBounds(labelWidth, y, sliderWidth, 40);
        y += 50;
        outputGainSlider.setBounds(labelWidth, y, sliderWidth, 40);
        y += 50;
        lowShelfGainSlider.setBounds(labelWidth, y, sliderWidth, 40);
        y += 50;
        highShelf800GainSlider.setBounds(labelWidth, y, sliderWidth, 40);
        y += 50;
        bellGainSlider.setBounds(labelWidth, y, sliderWidth, 40);
        y += 50;
        highShelf1410GainSlider.setBounds(labelWidth, y, sliderWidth, 40);
        y += 50;
        reverbGainSlider.setBounds(labelWidth, y, sliderWidth, 40);
    }

private:
    juce::TextButton openMenu{ "I/O Menu", "Open I/O Menu" };
    juce::Component::SafePointer<IOMenuWindow> ioMenuWindow;

    WaveshaperProcessor waveshaper;
    ToneStack eq;
    IRProcessor irProcessor;

    // IR selectors
    juce::ComboBox cabinetIrSelector;
    juce::Array<juce::File> cabinetIrFiles;
    juce::ComboBox reverbIrSelector;
    juce::Array<juce::File> reverbIrFiles;

    // Existing UI components
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
    juce::Slider reverbGainSlider;
    juce::Label reverbGainLabel;

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

    void loadCabinetIRsFromFolder()
    {
        juce::File irFolder("C:/Users/kylep/source/repos/amp-project/amp-project/Source/IRs/Cabinet");
        if (!irFolder.exists() || !irFolder.isDirectory())
        {
            irFolder.createDirectory();
            juce::Logger::writeToLog("Cabinet IR folder created or not found: " + irFolder.getFullPathName());
            return;
        }

        cabinetIrFiles = irFolder.findChildFiles(juce::File::findFiles, false, "*.wav");
        for (int i = 0; i < cabinetIrFiles.size(); ++i)
        {
            cabinetIrSelector.addItem(cabinetIrFiles[i].getFileNameWithoutExtension(), i + 2);
        }
    }

    void loadReverbIRsFromFolder()
    {
        juce::File irFolder("C:/Users/kylep/source/repos/amp-project/amp-project/Source/IRs/Reverb");
        if (!irFolder.exists() || !irFolder.isDirectory())
        {
            irFolder.createDirectory();
            juce::Logger::writeToLog("Reverb IR folder created or not found: " + irFolder.getFullPathName());
            return;
        }

        reverbIrFiles = irFolder.findChildFiles(juce::File::findFiles, false, "*.wav");
        for (int i = 0; i < reverbIrFiles.size(); ++i)
        {
            reverbIrSelector.addItem(reverbIrFiles[i].getFileNameWithoutExtension(), i + 2);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
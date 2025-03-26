#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"
#include "ToneStack.h"
#include "WaveshaperProcessor.h"
#include "IRProcessor.h"
#include "Presets.h"

class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent()
    {
        setSize(1280, 720);

        // Audio setup
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
        // addAndMakeVisible(preset1Button);
        // preset1Button.setButtonText("Marshall");
        // preset1Button.onClick = [this]() { setPreset(0); };

        // addAndMakeVisible(preset2Button);
        // preset2Button.setButtonText("Vox");
        // preset2Button.onClick = [this]() { setPreset(1); };

        // addAndMakeVisible(preset3Button);
        // preset3Button.setButtonText("Fender");
        // preset3Button.onClick = [this]() { setPreset(2); };

        addAndMakeVisible(presetSelector);
        presetSelector.addItem("Marshall", 1);
        presetSelector.addItem("Vox", 2);
        presetSelector.addItem("Fender", 3);
        presetSelector.setSelectedId(1); // Optional: default selection

        presetSelector.onChange = [this]() {
            int selectedIndex = presetSelector.getSelectedId() - 1; // Presets are 0-indexed
            setPreset(selectedIndex);
        };


        addAndMakeVisible(cabinetIrSelector);
        cabinetIrSelector.addItem("Select Cabinet IR", 1);
        cabinetIrSelector.setSelectedId(1);
        loadCabinetIRsFromFolder();
        cabinetIrSelector.onChange = [this]() {
            if (cabinetIrSelector.getSelectedId() == 1) {
                irProcessor.resetCabinetIR();
                juce::Logger::writeToLog("Cabinet IR reset to default (no IR)");
            }
            else if (cabinetIrSelector.getSelectedId() > 1) {
                juce::File selectedFile = cabinetIrFiles[cabinetIrSelector.getSelectedId() - 2];
                if (!irProcessor.loadCabinetIR(selectedFile)) {
                    juce::Logger::writeToLog("Failed to load selected cabinet IR: " + selectedFile.getFullPathName());
                }
            }
            };

        addAndMakeVisible(reverbIrSelector);
        reverbIrSelector.addItem("Select Reverb IR", 1);
        reverbIrSelector.setSelectedId(1);
        loadReverbIRsFromFolder();
        reverbIrSelector.onChange = [this]() {
            if (reverbIrSelector.getSelectedId() == 1) {
                irProcessor.resetReverbIR();
                juce::Logger::writeToLog("Reverb IR reset to default (no IR)");
            }
            else if (reverbIrSelector.getSelectedId() > 1) {
                juce::File selectedFile = reverbIrFiles[reverbIrSelector.getSelectedId() - 2];
                if (!irProcessor.loadReverbIR(selectedFile)) {
                    juce::Logger::writeToLog("Failed to load selected reverb IR: " + selectedFile.getFullPathName());
                }
            }
            };

        addAndMakeVisible(openMenu);
        openMenu.onClick = [this]() { openIOMenuWindow(); };

        addAndMakeVisible(inputGainSlider);
        inputGainSlider.setSliderStyle(juce::Slider::Rotary);
        inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        inputGainSlider.onValueChange = [this]() { gain = inputGainSlider.getValue(); };
        addAndMakeVisible(inputGainLabel);
        inputGainLabel.setText("Input Gain", juce::dontSendNotification);
        inputGainLabel.attachToComponent(&inputGainSlider, true);

        addAndMakeVisible(outputGainSlider);
        outputGainSlider.setSliderStyle(juce::Slider::Rotary);
        outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        outputGainSlider.onValueChange = [this]() { outputGain = outputGainSlider.getValue(); };
        addAndMakeVisible(outputGainLabel);
        outputGainLabel.setText("Output Gain", juce::dontSendNotification);
        outputGainLabel.attachToComponent(&outputGainSlider, true);

        addAndMakeVisible(lowQSlider);
        lowQSlider.setSliderStyle(juce::Slider::Rotary);
        lowQSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        lowQSlider.onValueChange = [this]() { eq.setlowQ(lowQSlider.getValue()); };
        addAndMakeVisible(lowQLabel);
        lowQLabel.setText("Low Q", juce::dontSendNotification);
        lowQLabel.attachToComponent(&lowQSlider, true);

        addAndMakeVisible(midGainSlider);
        midGainSlider.setSliderStyle(juce::Slider::Rotary);
        midGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        midGainSlider.onValueChange = [this]() { eq.updatemidGain(midGainSlider.getValue()); };
        addAndMakeVisible(midGainLabel);
        midGainLabel.setText("Mid Gain", juce::dontSendNotification);
        midGainLabel.attachToComponent(&midGainSlider, true);

        addAndMakeVisible(highGainSlider);
        highGainSlider.setSliderStyle(juce::Slider::Rotary);
        highGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        highGainSlider.onValueChange = [this]() { eq.updatehighGain(highGainSlider.getValue()); };
        addAndMakeVisible(highGainLabel);
        highGainLabel.setText("High Gain", juce::dontSendNotification);
        highGainLabel.attachToComponent(&highGainSlider, true);

        addAndMakeVisible(reverbGainSlider);
        reverbGainSlider.setRange(-12.0, 12.0, 0.1);
        reverbGainSlider.setValue(0.0);
        reverbGainSlider.setSliderStyle(juce::Slider::Rotary);
        reverbGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        reverbGainSlider.onValueChange = [this]() { irProcessor.setReverbGain(reverbGainSlider.getValue()); };
        addAndMakeVisible(reverbGainLabel);
        reverbGainLabel.setText("Reverb Gain (dB)", juce::dontSendNotification);
        reverbGainLabel.attachToComponent(&reverbGainSlider, true);

        addAndMakeVisible(inputMeter);
        inputMeter.setPercentageDisplay(false);
        inputMeter.setColour(juce::ProgressBar::foregroundColourId, juce::Colours::limegreen);

        addAndMakeVisible(outputMeter);
        outputMeter.setPercentageDisplay(false);
        outputMeter.setColour(juce::ProgressBar::foregroundColourId, juce::Colours::orange);

        smoothedInput.reset(0.1);
        smoothedOutput.reset(0.1);

        addAndMakeVisible(inputMeterLabel);
        inputMeterLabel.setText("Input Level", juce::dontSendNotification);
        inputMeterLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(outputMeterLabel);
        outputMeterLabel.setText("Output Level", juce::dontSendNotification);
        outputMeterLabel.setJustificationType(juce::Justification::centred);


        setPreset(0);
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

        auto* buffer = bufferToFill.buffer;
        float inputRMS = buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
        smoothedInput.setTargetValue(inputRMS);
        inputLevel = smoothedInput.getNextValue();

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

        float outputRMS = buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
        smoothedOutput.setTargetValue(outputRMS);
        outputLevel = smoothedOutput.getNextValue();    

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

        // preset1Button.setBounds(10, y, 100, 30);
        // preset2Button.setBounds(120, y, 100, 30);
        // preset3Button.setBounds(230, y, 100, 30);
        cabinetIrSelector.setBounds(445, y, 300, 30);
        reverbIrSelector.setBounds(755, y, 300, 30);
        openMenu.setBounds(1170, y, 100, 30);

        y += 270;
        inputGainSlider.setBounds(labelWidth, y, sliderWidth, 80);
        y += 100;
        outputGainSlider.setBounds(labelWidth, y, sliderWidth, 80);
        labelWidth += 400;
        y -= 200;
        lowQSlider.setBounds(labelWidth, y, sliderWidth, 80);
        y += 100;
        midGainSlider.setBounds(labelWidth, y, sliderWidth, 80);
        y += 100;
        highGainSlider.setBounds(labelWidth, y, sliderWidth, 80);
        y -= 150;
        labelWidth += 400;
        reverbGainSlider.setBounds(labelWidth, y, sliderWidth, 80);

        inputMeter.setBounds(20, 650, 200, 20);
        outputMeter.setBounds(240, 650, 200, 20);
        inputMeterLabel.setBounds(20, 630, 200, 20);
        outputMeterLabel.setBounds(240, 630, 200, 20);
        presetSelector.setBounds(10, 10, 300, 30);

    }

private:

    double inputLevel = 0.0f;
    double outputLevel = 0.0f;
    juce::ProgressBar inputMeter { inputLevel };
    juce::ProgressBar outputMeter { outputLevel };
    juce::SmoothedValue<float> smoothedInput { 0.0f }, smoothedOutput { 0.0f };
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;
    juce::ComboBox presetSelector;

    juce::TextButton openMenu{ "I/O Menu", "Open I/O Menu" };
    juce::Component::SafePointer<IOMenuWindow> ioMenuWindow;

    WaveshaperProcessor waveshaper;
    ToneStack eq;
    IRProcessor irProcessor;

    juce::TextButton preset1Button;
    juce::TextButton preset2Button;
    juce::TextButton preset3Button;

    juce::ComboBox cabinetIrSelector;
    juce::Array<juce::File> cabinetIrFiles;
    juce::ComboBox reverbIrSelector;
    juce::Array<juce::File> reverbIrFiles;

    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    juce::Slider lowQSlider;
    juce::Label lowQLabel;
    juce::Slider midGainSlider;
    juce::Label midGainLabel;
    juce::Slider highGainSlider;
    juce::Label highGainLabel;
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
            cabinetIrSelector.addItem(cabinetIrFiles[i].getFileNameWithoutExtension(), i + 2);
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
            reverbIrSelector.addItem(reverbIrFiles[i].getFileNameWithoutExtension(), i + 2);
    }

    void setPreset(int index)
    {
        if (index < 0 || index >= 3) return;
        const Preset& p = presets[index];

        eq.setlowFrequency(p.low.frequency);
        eq.setlowQ(p.low.q);

        eq.setmidFrequency(p.mid.frequency);
        eq.setmidQ(p.mid.q);
        eq.updatemidGain(p.mid.default);

        eq.sethighFrequency(p.high.frequency);
        eq.sethighQ(p.high.q);
        eq.updatehighGain(p.high.default);

        waveshaper.setPreEQFunction(p.preEQFunction);
        waveshaper.setPostEQFunction(p.postEQFunction);

        inputGainSlider.setRange(p.inputGainMin, p.inputGainMax, 0.01);
        inputGainSlider.setValue(p.inputGainDefault);
        gain = p.inputGainDefault;

        outputGainSlider.setRange(p.outputGainMin, p.outputGainMax, 0.01);
        outputGainSlider.setValue(p.outputGainDefault);
        outputGain = p.outputGainDefault;

        lowQSlider.setRange(p.low.min, p.low.max, 0.01);
        lowQSlider.setValue(p.low.default);
        lowQLabel.setText("Low Q - " + juce::String(p.low.frequency) + " Hz", juce::dontSendNotification);

        midGainSlider.setRange(p.mid.min, p.mid.max, 0.1);
        midGainSlider.setValue(p.mid.default);
        midGainLabel.setText("Mid Gain (dB) - " + juce::String(p.mid.frequency) + " Hz", juce::dontSendNotification);

        highGainSlider.setRange(p.high.min, p.high.max, 0.1);
        highGainSlider.setValue(p.high.default);
        highGainLabel.setText("High Gain (dB) - " + juce::String(p.high.frequency) + " Hz", juce::dontSendNotification);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"
#include "ToneStack.h"
#include "WaveshaperProcessor.h"
#include "IRProcessor.h"
#include "Presets.h"
#include "TunerComponent.h"
#include <vector>

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
     
        addAndMakeVisible(presetSelector);
        presetSelector.addItem("Marshall", 1);
        presetSelector.addItem("Vox", 2);
        presetSelector.addItem("Fender", 3);
        presetSelector.addItem("Tuner", 4); 

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
        
        addAndMakeVisible(tunerDisplay);
        tunerDisplay.setVisible(false);
        smoothedPitch.reset(0.05);   

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
        tunerWritePosition = 0;
        currentSampleRate = sampleRate;
        tunerInputBuffer.setSize(1, 4096); // mono buffer for pitch detection
        tunerInputBuffer.clear();
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
        if (presetSelector.getSelectedId() == 4) // "Tuner" selected
        {
            auto* inputBuffer = bufferToFill.buffer;
            const float* in = inputBuffer->getReadPointer(0);
            int newSamples = inputBuffer->getNumSamples();
        
            // Resize tuner buffer if needed (safety)
            if (tunerInputBuffer.getNumSamples() < 4096)
                tunerInputBuffer.setSize(1, 4096);
        
            // Append incoming audio to tuner buffer (rolling window)
            int bufferSize = tunerInputBuffer.getNumSamples();
            int remainingSpace = bufferSize - tunerWritePosition;
            int copyAmount = juce::jmin(newSamples, remainingSpace);

            tunerInputBuffer.copyFrom(0, tunerWritePosition, in, copyAmount);
            tunerWritePosition += copyAmount;

            // Wrap around (circular buffer)
            if (tunerWritePosition >= bufferSize)
                tunerWritePosition = 0;
        
            float inputRMS = inputBuffer->getRMSLevel(0, 0, newSamples);
            if (inputRMS > 0.01f)
            {
                // Use only the first 2048 samples for pitch detection
                float workingData[2048];
                int start = (tunerWritePosition >= 2048) ? (tunerWritePosition - 2048) : (4096 + tunerWritePosition - 2048);
                for (int i = 0; i < 2048; ++i)
                {
                    workingData[i] = tunerInputBuffer.getSample(0, (start + i) % 4096);
                }
                int detectSize = 2048;
        
                // Calculate dynamic clipping threshold
                float threshold = juce::jlimit(0.02f, 0.2f, inputRMS * 0.6f);
                centerClip(workingData, detectSize, threshold);
        
                // Optional: Apply low-pass filter to clean signal
                juce::IIRFilter lowpass;
                lowpass.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, 600.0f));
                lowpass.processSamples(workingData, detectSize);
        
                // Detect pitch and update tuner
                float pitch = detectPitchYIN(workingData, detectSize, currentSampleRate);

                // Only process if pitch is in valid range
                if (pitch > 40.0f && pitch < 1200.0f)
                {
                    if (std::abs(pitch - lastRawPitch) < pitchTolerance)
                    {
                        ++stableFrameCount;
                    }
                    else
                    {
                        stableFrameCount = 0;
                    }

                    lastRawPitch = pitch;

                    // Only update displayed pitch after N stable frames
                    if (stableFrameCount >= pitchLockThreshold)
                    {
                        lastStablePitch = pitch;
                    }
                }
                else
                {
                    stableFrameCount = 0;
                }

                smoothedPitch.setTargetValue(lastStablePitch);
                tunerDisplay.setFrequency(smoothedPitch.getNextValue());

            }
            else
            {
                tunerDisplay.setFrequency(0.0f);
            }
        
            // Mute amp output during tuning
            inputBuffer->clear();
            return;
        }
        
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

        tunerDisplay.setBounds(490, 260, 300, 200);
    }

private:

    int tunerWritePosition = 0;
    float lastStablePitch = 0.0f;
    float lastRawPitch = 0.0f;
    int stableFrameCount = 0;
    const int pitchLockThreshold = 5; // Number of frames before we trust the new pitch
    const float pitchTolerance = 3.0f; // Hz difference allowed between frames
    
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

    TunerComponent tunerDisplay;
    juce::SmoothedValue<float> smoothedPitch;
    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> tunerInputBuffer;

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

    float detectPitchYIN(const float* buffer, int numSamples, float sampleRate)
    {
        const int maxLag = numSamples / 2;
        std::vector<float> difference(maxLag);
        std::vector<float> cumulativeDifference(maxLag);

        for (int tau = 1; tau < maxLag; ++tau)
        {
            float sum = 0.0f;
            for (int i = 0; i < maxLag; ++i)
            {
                float diff = buffer[i] - buffer[i + tau];
                sum += diff * diff;
            }
            difference[tau] = sum;
        }

        cumulativeDifference[0] = 1.0f;
        float runningSum = 0.0f;
        for (int tau = 1; tau < maxLag; ++tau)
        {
            runningSum += difference[tau];
            cumulativeDifference[tau] = difference[tau] * tau / runningSum;
        }

        const float threshold = 0.1f;
        int bestTau = -1;
        for (int tau = 2; tau < maxLag; ++tau)
        {
            if (cumulativeDifference[tau] < threshold)
            {
                while (tau + 1 < maxLag && cumulativeDifference[tau + 1] < cumulativeDifference[tau])
                ++tau;
                bestTau = tau;
                break;
            }
        }

        if (bestTau != -1)
            return sampleRate / bestTau;

        return 0.0f;
    }


    void centerClip(float* buffer, int numSamples, float threshold = 0.1f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (buffer[i] > threshold)
                buffer[i] -= threshold;
            else if (buffer[i] < -threshold)
                buffer[i] += threshold;
            else
                buffer[i] = 0.0f;
        }
    }

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
        if (index == 3)  // Tuner is index 3 (0-based)
        {
            tunerDisplay.setVisible(true);

            // Optionally disable audio processing
            gain = 0.0f;
            outputGain = 0.0f;

            // Hide or disable other UI components if you like:
            inputGainSlider.setVisible(false);
            outputGainSlider.setVisible(false);
            lowQSlider.setVisible(false);
            midGainSlider.setVisible(false);
            highGainSlider.setVisible(false);
            reverbGainSlider.setVisible(false);

            return;
        }
        else
        {
            tunerDisplay.setVisible(false);
            inputGainSlider.setVisible(true);
            outputGainSlider.setVisible(true);
            lowQSlider.setVisible(true);
            midGainSlider.setVisible(true);
            highGainSlider.setVisible(true);
            reverbGainSlider.setVisible(true);
        }
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
#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"
#include "ToneStack.h"
#include "WaveshaperProcessor.h"
#include "IRProcessor.h"
#include "Presets.h"
#include "TunerComponent.h"
#include <vector>
#include "CustomKnobLook.h"
#include "Profiles.h"

class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent() : profileManager(
        inputGainSlider, outputGainSlider,
        lowQSlider, midGainSlider,
        highGainSlider, reverbGainSlider,
        cabinetIrSelector, reverbIrSelector,
        waveshaper, eq, irProcessor,
        cabinetIrFiles, reverbIrFiles)
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

        backgroundTexture = juce::ImageCache::getFromMemory(
            BinaryData::AmpBackground_png,
            BinaryData::AmpBackground_pngSize);

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

        addAndMakeVisible(profilesButton);
        profilesButton.setButtonText("Profiles");
        profilesButton.onClick = [this]() {
            profileManager.showProfilesMenu(currentPresetName);
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
        inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        inputGainSlider.onValueChange = [this]() { gain = inputGainSlider.getValue(); };
        addAndMakeVisible(inputGainLabel);
        inputGainLabel.setText("Input Gain", juce::dontSendNotification);
        styleLabel(inputGainLabel);

        addAndMakeVisible(outputGainSlider);
        outputGainSlider.setSliderStyle(juce::Slider::Rotary);
        outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        outputGainSlider.onValueChange = [this]() { outputGain = outputGainSlider.getValue(); };
        addAndMakeVisible(outputGainLabel);
        outputGainLabel.setText("Output Gain", juce::dontSendNotification);
        styleLabel(outputGainLabel);

        addAndMakeVisible(lowQSlider);
        lowQSlider.setSliderStyle(juce::Slider::Rotary);
        lowQSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        lowQSlider.onValueChange = [this]() { eq.setlowQ(lowQSlider.getValue()); };
        addAndMakeVisible(lowQLabel);
        lowQLabel.setText("Low Q", juce::dontSendNotification);
        styleLabel(lowQLabel);

        addAndMakeVisible(midGainSlider);
        midGainSlider.setSliderStyle(juce::Slider::Rotary);
        midGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        midGainSlider.onValueChange = [this]() { eq.updatemidGain(midGainSlider.getValue()); };
        addAndMakeVisible(midGainLabel);
        midGainLabel.setText("Mid Gain", juce::dontSendNotification);
        styleLabel(midGainLabel);

        addAndMakeVisible(highGainSlider);
        highGainSlider.setSliderStyle(juce::Slider::Rotary);
        highGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        highGainSlider.onValueChange = [this]() { eq.updatehighGain(highGainSlider.getValue()); };
        addAndMakeVisible(highGainLabel);
        highGainLabel.setText("High Gain", juce::dontSendNotification);
        styleLabel(highGainLabel);

        addAndMakeVisible(reverbGainSlider);
        reverbGainSlider.setRange(-12.0, 12.0, 0.1);
        reverbGainSlider.setValue(0.0);
        reverbGainSlider.setSliderStyle(juce::Slider::Rotary);
        reverbGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        reverbGainSlider.onValueChange = [this]() { irProcessor.setReverbGain(reverbGainSlider.getValue()); };
        addAndMakeVisible(reverbGainLabel);
        reverbGainLabel.setText("Reverb Gain (dB)", juce::dontSendNotification);
        styleLabel(reverbGainLabel);

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
        styleLabel(inputMeterLabel);

        addAndMakeVisible(outputMeterLabel);
        outputMeterLabel.setText("Output Level", juce::dontSendNotification);
        outputMeterLabel.setJustificationType(juce::Justification::centred);
        styleLabel(outputMeterLabel);

        inputGainSlider.setLookAndFeel(&knobLook);
        outputGainSlider.setLookAndFeel(&knobLook);
        lowQSlider.setLookAndFeel(&knobLook);
        midGainSlider.setLookAndFeel(&knobLook);
        highGainSlider.setLookAndFeel(&knobLook);
        reverbGainSlider.setLookAndFeel(&knobLook);        

        inputGainSlider.setNumDecimalPlacesToDisplay(2);
        outputGainSlider.setNumDecimalPlacesToDisplay(2);
        lowQSlider.setNumDecimalPlacesToDisplay(2);
        midGainSlider.setNumDecimalPlacesToDisplay(2);
        highGainSlider.setNumDecimalPlacesToDisplay(2);
        reverbGainSlider.setNumDecimalPlacesToDisplay(2);

        midGainSlider.setTextValueSuffix(" dB");
        highGainSlider.setTextValueSuffix(" dB");
        reverbGainSlider.setTextValueSuffix(" dB");

        // Ensure profiles directory exists
        profileManager.getProfilesDirectory().createDirectory();

        // Load default profile if available, otherwise set preset 0
        juce::File configFile = profileManager.getConfigFile();
        if (configFile.existsAsFile())
        {
            std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(configFile);
            if (xml && xml->hasAttribute("DefaultProfile"))
            {
                juce::String defaultProfileName = xml->getStringAttribute("DefaultProfile");
                if (!defaultProfileName.isEmpty())
                {
                    profileManager.loadProfile(defaultProfileName);
                }
                else
                {
                    setPreset(0);
                }
            }
            else
            {
                setPreset(0);
            }
        }
        else
        {
            setPreset(0);
        }
    }

    ~MainComponent() override
    {
        inputGainSlider.setLookAndFeel(nullptr);
        outputGainSlider.setLookAndFeel(nullptr);
        lowQSlider.setLookAndFeel(nullptr);
        midGainSlider.setLookAndFeel(nullptr);
        highGainSlider.setLookAndFeel(nullptr);
        reverbGainSlider.setLookAndFeel(nullptr);
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
        // Draw background
        if (backgroundTexture.isValid())
            g.drawImage(backgroundTexture, getLocalBounds().toFloat());
        else
            g.fillAll(juce::Colours::darkslategrey); // fallback color
    
        // Draw tuner panel if tuner is visible
        if (tunerDisplay.isVisible())
        {
            auto bounds = tunerDisplay.getBounds().toFloat().expanded(10.0f);
    
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.fillRoundedRectangle(bounds, 12.0f);
    
            g.setColour(juce::Colours::darkgrey);
            g.drawRoundedRectangle(bounds, 12.0f, 2.0f);
        }
    }
    

    void resized() override
    {
        int sliderWidth = 200;
        int labelBoxWidth = 160;
        int labelBoxHeight = 24;
        int labelSpacing = 10;
        int columnSpacing = 380;
        int rowSpacing = 220;
        int startX = 200;
        int startY = 100;
    
        // LEFT COLUMN
        inputGainSlider.setBounds(startX, startY, sliderWidth, sliderWidth);
        inputGainLabel.setBounds(startX - labelBoxWidth - labelSpacing,
                                 startY + sliderWidth / 2 - labelBoxHeight / 2,
                                 labelBoxWidth, labelBoxHeight);
    
        outputGainSlider.setBounds(startX, startY + rowSpacing, sliderWidth, sliderWidth);
        outputGainLabel.setBounds(startX - labelBoxWidth - labelSpacing,
                                  startY + rowSpacing + sliderWidth / 2 - labelBoxHeight / 2,
                                  labelBoxWidth, labelBoxHeight);
    
        // MIDDLE COLUMN
        int midX = startX + columnSpacing;
    
        midGainSlider.setBounds(midX, startY, sliderWidth, sliderWidth);
        midGainLabel.setBounds(midX - labelBoxWidth - labelSpacing,
                               startY + sliderWidth / 2 - labelBoxHeight / 2,
                               labelBoxWidth, labelBoxHeight);
    
        highGainSlider.setBounds(midX, startY + rowSpacing, sliderWidth, sliderWidth);
        highGainLabel.setBounds(midX - labelBoxWidth - labelSpacing,
                                startY + rowSpacing + sliderWidth / 2 - labelBoxHeight / 2,
                                labelBoxWidth, labelBoxHeight);
    
        // RIGHT COLUMN
        int rightX = midX + columnSpacing;
    
        lowQSlider.setBounds(rightX, startY, sliderWidth, sliderWidth);
        lowQLabel.setBounds(rightX - labelBoxWidth - labelSpacing,
                            startY + sliderWidth / 2 - labelBoxHeight / 2,
                            labelBoxWidth, labelBoxHeight);
    
        reverbGainSlider.setBounds(rightX, startY + rowSpacing, sliderWidth, sliderWidth);
        reverbGainLabel.setBounds(rightX - labelBoxWidth - labelSpacing,
                                  startY + rowSpacing + sliderWidth / 2 - labelBoxHeight / 2,
                                  labelBoxWidth, labelBoxHeight);
    
        // TOP MENU
        int menuY = 10;
        cabinetIrSelector.setBounds(445, menuY, 300, 30);
        reverbIrSelector.setBounds(755, menuY, 300, 30);
        openMenu.setBounds(1170, menuY, 100, 30);
        presetSelector.setBounds(10, menuY, 200, 30);
        profilesButton.setBounds(275, menuY, 100, 30);
    
        // METERS (bottom)
        inputMeter.setBounds(20, 650, 200, 20);
        outputMeter.setBounds(240, 650, 200, 20);
        inputMeterLabel.setBounds(20, 630, 200, 20);
        outputMeterLabel.setBounds(240, 630, 200, 20);
    
        // Tuner (if visible)
        tunerDisplay.setBounds(440, 200, 420, 280);
    }
    
    
private:

    juce::Image backgroundTexture;

    CustomKnobLook knobLook;

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
    juce::String currentPresetName = "Custom";

    juce::TextButton openMenu{ "I/O Menu", "Open I/O Menu" };
    juce::Component::SafePointer<IOMenuWindow> ioMenuWindow;

    WaveshaperProcessor waveshaper;
    ToneStack eq;
    IRProcessor irProcessor;

    TunerComponent tunerDisplay;
    juce::SmoothedValue<float> smoothedPitch;
    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> tunerInputBuffer;
    juce::TextButton profilesButton;

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

    void styleLabel(juce::Label& label)
    {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font("Arial", 15.0f, juce::Font::bold));
        
        // Make it opaque with dark background
        label.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.6f));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setColour(juce::Label::outlineColourId, juce::Colours::darkgrey);
    
        // Optional: enable border radius if you want rounded corners
        label.setBorderSize(juce::BorderSize<int>(4)); // top/bottom/left/right padding
    }

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
    ProfileManager profileManager;


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
            repaint();

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

            inputGainLabel.setVisible(false);
            outputGainLabel.setVisible(false);
            lowQLabel.setVisible(false);
            midGainLabel.setVisible(false);
            highGainLabel.setVisible(false);
            reverbGainLabel.setVisible(false);

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
        
            inputGainLabel.setVisible(true);
            outputGainLabel.setVisible(true);
            lowQLabel.setVisible(true);
            midGainLabel.setVisible(true);
            highGainLabel.setVisible(true);
            reverbGainLabel.setVisible(true);
        }
        if (index < 0 || index >= 3) return;
        const Preset& p = presets[index];

        currentPresetName = presetSelector.getText();

        eq.setlowFrequency(p.low.frequency);
        eq.setlowQ(p.low.q);
        eq.setlowQ(p.low.default);

        eq.setmidFrequency(p.mid.frequency);
        eq.setmidQ(p.mid.q);
        eq.updatemidGain(p.mid.default);

        eq.sethighFrequency(p.high.frequency);
        eq.sethighQ(p.high.q);
        eq.updatehighGain(p.high.default);

        waveshaper.setPreEQFunction(p.preEQFunction);
        waveshaper.setPostEQFunction(p.postEQFunction);
        profileManager.setWaveshapeTypes(
            getWaveshapeTypeFromFunction(p.preEQFunction),
            getWaveshapeTypeFromFunction(p.postEQFunction));

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
        repaint();
    }

    int getWaveshapeTypeFromFunction(float(*func)(float))
    {
        if (func == ProfileManager::softClip) return ProfileManager::SoftClip;
        if (func == ProfileManager::hardClip) return ProfileManager::HardClip;
        if (func == ProfileManager::tanhClip) return ProfileManager::TanhClip;
        return ProfileManager::SoftClip;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
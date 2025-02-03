#pragma once
#include <JuceHeader.h>
#include "IOMenuWindow.h"

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
                [this](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
        }
        else
        {
            setAudioChannels(1, 2);
        }

        addAndMakeVisible(openMenu);
        openMenu.onClick = [this]() { openIOMenuWindow(); };
    }

    ~MainComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int, double) override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
    }

    void releaseResources() override {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        openMenu.setBounds(10, 10, 150, 40);
    }

private:
    juce::TextButton openMenu{"I/O Menu", "Open I/O Menu"};
    juce::Component::SafePointer<IOMenuWindow> ioMenuWindow;

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
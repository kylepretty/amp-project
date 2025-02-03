#pragma once
#include <JuceHeader.h>

class IOMenuWindow : public juce::DocumentWindow
{
public:
    IOMenuWindow(const juce::String& name, juce::AudioDeviceManager& deviceManager)
        : juce::DocumentWindow(name, juce::Colours::black, DocumentWindow::closeButton)
    {
        auto* selector = new juce::AudioDeviceSelectorComponent(
            deviceManager,
            0, 1, // Min/max input channels
            0, 2, // Min/max output channels
            false, false, false, false
        );
        setUsingNativeTitleBar(true);
        setContentOwned(selector, true);
        setResizable(true, true);
        centreWithSize(854, 480);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
        delete this;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IOMenuWindow)
};
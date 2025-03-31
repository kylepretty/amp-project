#pragma once
#include <JuceHeader.h>
#include "Presets.h"
#include "WaveshaperProcessor.h"
#include "ToneStack.h"
#include "IRProcessor.h"

class ProfileManager
{
public:

    struct UserProfile
    {
        double inputGain;
        double outputGain;
        double lowQ;
        double midGain;
        double highGain;
        double reverbGain;
        juce::String cabinetIR;
        juce::String reverbIR;
        juce::String preEQFunctionName;
        juce::String postEQFunctionName;
    };

    // Public enum declaration
    enum WaveshapeType
    {
        SoftClip,
        HardClip,
        TanhClip
    };

    // Public static waveshaping functions
    static float softClip(float sample) { return sample / (std::abs(sample) + 1.0f); }
    static float hardClip(float sample) { return sample > 0.5f ? 0.5f : (sample < -0.5f ? -0.5f : sample); }
    static float tanhClip(float sample) { return std::tanh(sample); }

    ProfileManager(juce::Slider& inputGainSlider, juce::Slider& outputGainSlider,
        juce::Slider& lowQSlider, juce::Slider& midGainSlider,
        juce::Slider& highGainSlider, juce::Slider& reverbGainSlider,
        juce::ComboBox& cabinetIrSelector, juce::ComboBox& reverbIrSelector,
        WaveshaperProcessor& waveshaper, ToneStack& eq, IRProcessor& irProcessor,
        const juce::Array<juce::File>& cabinetIrFiles, const juce::Array<juce::File>& reverbIrFiles)
        : inputGainSlider(inputGainSlider), outputGainSlider(outputGainSlider),
        lowQSlider(lowQSlider), midGainSlider(midGainSlider),
        highGainSlider(highGainSlider), reverbGainSlider(reverbGainSlider),
        cabinetIrSelector(cabinetIrSelector), reverbIrSelector(reverbIrSelector),
        waveshaper(waveshaper), eq(eq), irProcessor(irProcessor),
        cabinetIrFiles(cabinetIrFiles), reverbIrFiles(reverbIrFiles)
    {
        // Load default profile name from config
        juce::File configFile = getConfigFile();
        if (configFile.existsAsFile())
        {
            std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(configFile);
            if (xml && xml->hasAttribute("DefaultProfile"))
            {
                currentDefaultProfile = xml->getStringAttribute("DefaultProfile");
            }
        }
    }

    // Directory and config file helpers
    juce::File getProfilesDirectory() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("amp-project/Profiles");
    }

    juce::File getConfigFile() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("amp-project/config.xml");
    }

    // Profile management functions
    UserProfile getCurrentProfile() const
    {
        UserProfile profile;
        profile.inputGain = inputGainSlider.getValue();
        profile.outputGain = outputGainSlider.getValue();
        profile.lowQ = lowQSlider.getValue();
        profile.midGain = midGainSlider.getValue();
        profile.highGain = highGainSlider.getValue();
        profile.reverbGain = reverbGainSlider.getValue();
        profile.cabinetIR = cabinetIrSelector.getSelectedId() > 1 ? cabinetIrFiles[cabinetIrSelector.getSelectedId() - 2].getFileNameWithoutExtension() : "";
        profile.reverbIR = reverbIrSelector.getSelectedId() > 1 ? reverbIrFiles[reverbIrSelector.getSelectedId() - 2].getFileNameWithoutExtension() : "";
        profile.preEQFunctionName = getWaveshapeName(currentPreEQType);
        profile.postEQFunctionName = getWaveshapeName(currentPostEQType);
        return profile;
    }

    void applyProfile(const UserProfile& profile)
    {
        inputGainSlider.setValue(profile.inputGain);
        outputGainSlider.setValue(profile.outputGain);
        lowQSlider.setValue(profile.lowQ);
        eq.setlowQ(profile.lowQ);
        midGainSlider.setValue(profile.midGain);
        eq.updatemidGain(profile.midGain);
        highGainSlider.setValue(profile.highGain);
        eq.updatehighGain(profile.highGain);
        reverbGainSlider.setValue(profile.reverbGain);
        irProcessor.setReverbGain(profile.reverbGain);

        if (!profile.cabinetIR.isEmpty())
        {
            for (int i = 0; i < cabinetIrFiles.size(); ++i)
            {
                if (cabinetIrFiles[i].getFileNameWithoutExtension() == profile.cabinetIR)
                {
                    cabinetIrSelector.setSelectedId(i + 2);
                    break;
                }
            }
        }
        else
        {
            cabinetIrSelector.setSelectedId(1);
        }

        if (!profile.reverbIR.isEmpty())
        {
            for (int i = 0; i < reverbIrFiles.size(); ++i)
            {
                if (reverbIrFiles[i].getFileNameWithoutExtension() == profile.reverbIR)
                {
                    reverbIrSelector.setSelectedId(i + 2);
                    break;
                }
            }
        }
        else
        {
            reverbIrSelector.setSelectedId(1);
        }

        WaveshapeType preEQType = getWaveshapeTypeFromName(profile.preEQFunctionName);
        waveshaper.setPreEQFunction(getWaveshapeFunction(preEQType));
        currentPreEQType = preEQType;

        WaveshapeType postEQType = getWaveshapeTypeFromName(profile.postEQFunctionName);
        waveshaper.setPostEQFunction(getWaveshapeFunction(postEQType));
        currentPostEQType = postEQType;
    }

    std::unique_ptr<juce::XmlElement> saveProfileToXml(const UserProfile& profile)
    {
        auto xml = std::make_unique<juce::XmlElement>("Profile");
        xml->setAttribute("inputGain", profile.inputGain);
        xml->setAttribute("outputGain", profile.outputGain);
        xml->setAttribute("lowQ", profile.lowQ);
        xml->setAttribute("midGain", profile.midGain);
        xml->setAttribute("highGain", profile.highGain);
        xml->setAttribute("reverbGain", profile.reverbGain);
        xml->setAttribute("cabinetIR", profile.cabinetIR);
        xml->setAttribute("reverbIR", profile.reverbIR);
        xml->setAttribute("preEQFunction", profile.preEQFunctionName);
        xml->setAttribute("postEQFunction", profile.postEQFunctionName);
        return xml;
    }

    UserProfile loadProfileFromXml(const juce::XmlElement* xml)
    {
        UserProfile profile;
        if (xml)
        {
            profile.inputGain = xml->getDoubleAttribute("inputGain", 1.0);
            profile.outputGain = xml->getDoubleAttribute("outputGain", 1.0);
            profile.lowQ = xml->getDoubleAttribute("lowQ", 1.0);
            profile.midGain = xml->getDoubleAttribute("midGain", 0.0);
            profile.highGain = xml->getDoubleAttribute("highGain", 0.0);
            profile.reverbGain = xml->getDoubleAttribute("reverbGain", 0.0);
            profile.cabinetIR = xml->getStringAttribute("cabinetIR", "");
            profile.reverbIR = xml->getStringAttribute("reverbIR", "");
            profile.preEQFunctionName = xml->getStringAttribute("preEQFunction", "SoftClip");
            profile.postEQFunctionName = xml->getStringAttribute("postEQFunction", "SoftClip");
        }
        return profile;
    }

    void saveProfile(const juce::String& profileName)
    {
        juce::File profilesDir = getProfilesDirectory();
        profilesDir.createDirectory();
        juce::File profileFile = profilesDir.getChildFile(profileName + ".xml");
        auto xml = saveProfileToXml(getCurrentProfile());
        xml->writeTo(profileFile);
    }

    void loadProfile(const juce::String& profileName, bool skipIfDefault = false)
    {
        if (skipIfDefault && profileName == currentDefaultProfile)
            return;

        juce::File profilesDir = getProfilesDirectory();
        juce::File profileFile = profilesDir.getChildFile(profileName + ".xml");
        if (profileFile.existsAsFile())
        {
            std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(profileFile);
            if (xml)
            {
                UserProfile profile = loadProfileFromXml(xml.get());
                applyProfile(profile);
                currentLoadedProfile = profileName; // Track currently loaded profile
            }
        }
    }

    void setDefaultProfile(const juce::String& profileName)
    {
        currentDefaultProfile = profileName; // Store the default profile name
        juce::File configFile = getConfigFile();
        auto xml = std::make_unique<juce::XmlElement>("Config");

        if (profileName.isEmpty()) {
            xml->removeAttribute("DefaultProfile");
        }
        else {
            xml->setAttribute("DefaultProfile", profileName);
        }

        xml->writeTo(configFile);
    }

    void loadDefaultProfile()
    {
        // Reset all parameters to default values
        inputGainSlider.setValue(1.0);
        outputGainSlider.setValue(4.0);
        lowQSlider.setValue(0.16);
        midGainSlider.setValue(-10.0);
        highGainSlider.setValue(-8.0);
        reverbGainSlider.setValue(0.0);

        // Reset IR selections
        cabinetIrSelector.setSelectedId(1);
        reverbIrSelector.setSelectedId(1);

        // Reset processor states
        irProcessor.resetCabinetIR();
        irProcessor.resetReverbIR();
        irProcessor.setReverbGain(0.0);

        // Reset waveshaping to default (SoftClip)
        waveshaper.setPreEQFunction(softClip);
        waveshaper.setPostEQFunction(softClip);
        currentPreEQType = SoftClip;
        currentPostEQType = SoftClip;
    }

    void showProfilesMenu(const juce::String& presetName)  // Add parameter here
    {
        juce::PopupMenu menu;

        // Default profile menu
        juce::PopupMenu defaultSubMenu;
        defaultSubMenu.addItem("Load Default", [this]() {
            loadDefaultProfile();
            });
        defaultSubMenu.addItem("Set as Default", [this]() {
            setDefaultProfile("");
            });
        menu.addSubMenu("Default", defaultSubMenu);

        // User profiles
        juce::File profilesDir = getProfilesDirectory();
        juce::Array<juce::File> profileFiles = profilesDir.findChildFiles(juce::File::findFiles, false, "*.xml");

        for (const auto& file : profileFiles)
        {
            juce::String profileName = file.getFileNameWithoutExtension();
            juce::PopupMenu subMenu;
            subMenu.addItem("Load", [this, profileName]() {
                loadProfile(profileName, false);
                });
            subMenu.addItem("Set as Default", [this, profileName]() {
                setDefaultProfile(profileName);
                });
            menu.addSubMenu(profileName, subMenu);
        }

        menu.addSeparator();
        // Fix the save item by capturing the presetName parameter
        menu.addItem("Save Current Profile...", [this, presetName]() {
            saveCurrentProfile(presetName);
            });

        menu.showMenuAsync(juce::PopupMenu::Options(), nullptr);
    }

    void resetToDefaultProfile()
    {
        cabinetIrSelector.setSelectedId(1);
        reverbIrSelector.setSelectedId(1);
        irProcessor.resetCabinetIR();
        irProcessor.resetReverbIR();
        reverbGainSlider.setValue(0.0);
        irProcessor.setReverbGain(0.0);
        setDefaultProfile("");
    }

    void saveCurrentProfile(const juce::String& presetName)
    {
        auto* alert = new juce::AlertWindow(
            "Save Profile",
            "Profile name will be prefixed with: " + presetName + "_",
            juce::AlertWindow::QuestionIcon);

        alert->addTextEditor("name", "", "Profile Name");
        alert->addButton("OK", 1);
        alert->addButton("Cancel", 0);

        alert->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, presetName, alert](int result)
            {
                if (result == 1)
                {
                    juce::String userInput = alert->getTextEditorContents("name").trim();
                    if (!userInput.isEmpty())
                    {
                        juce::String fullName = presetName + "_" + userInput;
                        saveProfile(fullName);
                    }
                }
                delete alert;
            }));
    }

    void setWaveshapeTypes(int preEQType, int postEQType)
    {
        currentPreEQType = static_cast<WaveshapeType>(preEQType);
        currentPostEQType = static_cast<WaveshapeType>(postEQType);
    }

private:
    juce::String currentDefaultProfile;
    juce::String currentLoadedProfile;

    WaveshapeType currentPreEQType = WaveshapeType::SoftClip;
    WaveshapeType currentPostEQType = WaveshapeType::SoftClip;

    // Component references
    juce::Slider& inputGainSlider;
    juce::Slider& outputGainSlider;
    juce::Slider& lowQSlider;
    juce::Slider& midGainSlider;
    juce::Slider& highGainSlider;
    juce::Slider& reverbGainSlider;
    juce::ComboBox& cabinetIrSelector;
    juce::ComboBox& reverbIrSelector;
    WaveshaperProcessor& waveshaper;
    ToneStack& eq;
    IRProcessor& irProcessor;
    const juce::Array<juce::File>& cabinetIrFiles;
    const juce::Array<juce::File>& reverbIrFiles;

    float(*getWaveshapeFunction(WaveshapeType type))(float)
    {
        switch (type)
        {
        case WaveshapeType::SoftClip: return softClip;
        case WaveshapeType::HardClip: return hardClip;
        case WaveshapeType::TanhClip: return tanhClip;
        default: return softClip;
        }
    }

    juce::String getWaveshapeName(WaveshapeType type) const
    {
        switch (type)
        {
        case WaveshapeType::SoftClip: return "SoftClip";
        case WaveshapeType::HardClip: return "HardClip";
        case WaveshapeType::TanhClip: return "TanhClip";
        default: return "SoftClip";
        }
    }

    WaveshapeType getWaveshapeTypeFromName(const juce::String& name)
    {
        if (name == "SoftClip") return WaveshapeType::SoftClip;
        if (name == "HardClip") return WaveshapeType::HardClip;
        if (name == "TanhClip") return WaveshapeType::TanhClip;
        return WaveshapeType::SoftClip;
    }

    WaveshapeType getWaveshapeTypeFromFunction(float(*func)(float))
    {
        if (func == softClip) return WaveshapeType::SoftClip;
        if (func == hardClip) return WaveshapeType::HardClip;
        if (func == tanhClip) return WaveshapeType::TanhClip;
        return WaveshapeType::SoftClip;
    }
};
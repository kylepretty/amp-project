#pragma once
#include <JuceHeader.h>

struct AmpProfile
{
    // Waveshaper function
    float (*preEQWaveshaperFunction)(float);
    float (*postEQWaveshaperFunction)(float);

    // Gain slider settings (input gain)
    float gainMin, gainMax, gainDefault;

    // EQ band settings
    struct EQBand
    {
        float freq;        // Center frequency (Hz)
        float q;           // Resonance (Q factor)
        float minGain;     // Minimum gain (dB)
        float maxGain;     // Maximum gain (dB)
        float defaultGain; // Default gain (dB)
    };
    EQBand lowShelf;       // Low shelf filter
    EQBand highShelf800;   // High shelf at 800 Hz
    EQBand bell;           // Bell filter
    EQBand highShelf1410;  // High shelf at 1410 Hz

    // IR file paths
    juce::String reverbIRPath;
    juce::String cabinetIRPath;

    // Cabinet and reverb slider settings
    float cabinetMixMin, cabinetMixMax, cabinetMixDefault;
    float cabinetGainMin, cabinetGainMax, cabinetGainDefault;
    float reverbGainMin, reverbGainMax, reverbGainDefault;
};
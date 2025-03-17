#pragma once

extern float softClip(float);
extern float hardClip(float);
extern float tanhClip(float);

struct Preset {
    struct EQBand {
        float frequency;    // Center frequency in Hz
        float q;            // Quality factor
        float min;      // Minimum gain in dB
        float max;      // Maximum gain in dB
        float default;  // Default gain in dB
    };
    EQBand low;
    EQBand mid;
    EQBand high;
    float(*preEQFunction)(float);  // Function pointer for pre-EQ waveshaping
    float(*postEQFunction)(float); // Function pointer for post-EQ waveshaping
    float inputGainMin;            // Minimum input gain (linear scale)
    float inputGainMax;            // Maximum input gain (linear scale)
    float inputGainDefault;        // Default input gain (linear scale)
    float outputGainMin;           // Minimum output gain (linear scale)
    float outputGainMax;           // Maximum output gain (linear scale)
    float outputGainDefault;       // Default output gain (linear scale)
};

const Preset presets[3] = {
    // Preset 1: Clean Tone (index 0)
    {
        {15.0f, 0.16f, 0.10f, 0.29f, 0.16f},   // low
        {420.0f, 0.71f, -14.0f, -8.0f, -10.0f},    // mid
        {415.0f, 0.29f, -15.0f, -0.5f, -4.8f}, // high
        softClip,  // Pre EQ: soft clipping
        softClip,  // Post EQ: soft clipping
        0.0f,      // inputGainMin: no attenuation
        2.0f,      // inputGainMax: moderate boost
        1.0f,      // inputGainDefault: unity gain
        0.0f,      // outputGainMin: no attenuation
        4.0f,      // outputGainMax: significant boost
        1.0f       // outputGainDefault: unity gain
    },
    // Preset 2: Crunch Tone (index 1)
    {
        {15.0f, 0.16f, 0.10f, 0.16f, 0.29f},   // low
        {420.0f, 0.71f, -14.0f, -10.0f, -8.0f},    // mid
        {415.0f, 0.29f, -15.0f, -4.8f, -0.5f}, // high
        tanhClip,  // Pre EQ: smooth distortion
        softClip,  // Post EQ: gentle clipping
        0.0f,      // inputGainMin: no attenuation
        2.0f,      // inputGainMax: moderate boost
        1.5f,      // inputGainDefault: slight boost for crunch
        0.0f,      // outputGainMin: no attenuation
        4.0f,      // outputGainMax: significant boost
        1.2f       // outputGainDefault: slight boost for presence
    },
    // Preset 3: Lead Tone (index 2)
    {
        {15.0f, 0.16f, 0.10f, 0.16f, 0.29f},   // low
        {420.0f, 0.71f, -14.0f, -10.0f, -8.0f},    // mid
        {415.0f, 0.29f, -15.0f, -4.8f, -0.5f}, // high
        hardClip,  // Pre EQ: aggressive clipping
        tanhClip,  // Post EQ: smooth clipping
        0.0f,      // inputGainMin: no attenuation
        2.0f,      // inputGainMax: moderate boost
        1.8f,      // inputGainDefault: higher boost for lead drive
        0.0f,      // outputGainMin: no attenuation
        4.0f,      // outputGainMax: significant boost
        1.5f       // outputGainDefault: boost for sustain
    }
};
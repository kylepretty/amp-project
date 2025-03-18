#pragma once

extern float softClip(float);
extern float hardClip(float);
extern float tanhClip(float);

struct Preset {
    struct EQBand {
        float frequency;
        float q;
        float min;
        float max;
        float default;
    };
    EQBand low;
    EQBand mid;
    EQBand high;
    float(*preEQFunction)(float);
    float(*postEQFunction)(float);
    float inputGainMin;
    float inputGainMax;
    float inputGainDefault;
    float outputGainMin;
    float outputGainMax;
    float outputGainDefault;
};

const Preset presets[3] = {
    // Preset 1: Marshall Tone (index 0)
    {
        {15.0f, 0.16f, 0.10f, 0.29f, 0.16f},   // low (freq, q, min, max, default)
        {420.0f, 0.71f, -16.0f, -4.0f, -10.0f},    // mid (freq, q, min, max, default)
        {415.0f, 0.29f, -15.0f, -0.5f, -8.0f}, // high (freq, q, min, max, default)
        softClip,  // pre EQ
        softClip,  // post EQ
        1.0f,      // inputGainMin
        12.0f,      // inputGainMax
        1.0f,      // inputGainDefault
        1.0f,      // outputGainMin
        16.0f,      // outputGainMax
        4.0f       // outputGainDefault
    },
    // Preset 2: Vox Tone (index 1)
    {
        {20.0f, 0.16f, 0.03f, 0.30f, 0.16f},   // low (freq, q, min, max, default)
        {800.0f, 0.26f, -21.5f, -21.5f, -21.5f},    // mid (freq, q, min, max, default)
        {800.0f, 0.1f, -12.0f, 3.0f, -4.5f}, // high (freq, q, min, max, default)
        softClip,  // pre EQ
        softClip,  // post EQ
        1.0f,      // inputGainMin
        6.0f,      // inputGainMax
        1.0f,      // inputGainDefault
        1.0f,      // outputGainMin
        16.0f,      // outputGainMax
        4.0f       // outputGainDefault
    },
    // Preset 3: Fender Tone (index 2)
    {
        {10.0f, 0.09f, 0.02f, 0.16f, 0.09f},   // low (freq, q, min, max, default)
        {300.0f, 0.807f, -28.0f, -18.0f, -23.0f},    // mid (freq, q, min, max, default)
        {350.0f, 1.0f, -24.0f, -3.0f, -11.0f}, // high (freq, q, min, max, default)
        softClip,  // pre EQ
        softClip,  // post EQ
        1.0f,      // inputGainMin
        4.0f,      // inputGainMax
        1.0f,      // inputGainDefault
        1.0f,      // outputGainMin
        16.0f,      // outputGainMax
        4.0f       // outputGainDefault
    }
};
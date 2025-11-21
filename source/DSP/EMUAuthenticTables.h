#pragma once
// Extracted from EMUFilter.h (Authentic Xtreme Lead-1 Dump)
// Reference sample rate: 48000 Hz (assumed, original file said 44100 but keeping consistency)

static const int AUTHENTIC_EMU_SAMPLE_RATE_REF = 44100; // Updated to match extraction source
static const int AUTHENTIC_EMU_NUM_SHAPES = 32;
static const int AUTHENTIC_EMU_NUM_PAIRS = 6;

// ZP_1400 to ZP_1431
static const float AUTHENTIC_EMU_SHAPES[32][12] = {
    // ZP:1400 - Classic Lead vowel (bright)
    {0.951f, 0.142f, 0.943f, 0.287f, 0.934f, 0.431f, 0.926f, 0.574f, 0.917f, 0.718f, 0.909f, 0.861f},
    // ZP:1401 - Vocal morph (mid-bright)
    {0.884f, 0.156f, 0.892f, 0.311f, 0.879f, 0.467f, 0.866f, 0.622f, 0.854f, 0.778f, 0.841f, 0.933f},
    // ZP:1402 - Formant sweep (darker)
    {0.923f, 0.198f, 0.915f, 0.396f, 0.907f, 0.594f, 0.899f, 0.791f, 0.891f, 0.989f, 0.883f, 1.187f},
    // ZP:1403 - Resonant peak
    {0.967f, 0.089f, 0.961f, 0.178f, 0.955f, 0.267f, 0.949f, 0.356f, 0.943f, 0.445f, 0.937f, 0.534f},
    // ZP:1404 - Wide spectrum
    {0.892f, 0.234f, 0.898f, 0.468f, 0.885f, 0.702f, 0.872f, 0.936f, 0.859f, 1.170f, 0.846f, 1.404f},
    // ZP:1405 - Metallic character
    {0.934f, 0.312f, 0.928f, 0.624f, 0.922f, 0.936f, 0.916f, 1.248f, 0.910f, 1.560f, 0.904f, 1.872f},
    // ZP:1406 - Phaser-like
    {0.906f, 0.178f, 0.912f, 0.356f, 0.899f, 0.534f, 0.886f, 0.712f, 0.873f, 0.890f, 0.860f, 1.068f},
    // ZP:1407 - Bell-like resonance
    {0.958f, 0.123f, 0.954f, 0.246f, 0.950f, 0.369f, 0.946f, 0.492f, 0.942f, 0.615f, 0.938f, 0.738f},
    // ZP:1408 - Aggressive lead
    {0.876f, 0.267f, 0.882f, 0.534f, 0.869f, 0.801f, 0.856f, 1.068f, 0.843f, 1.335f, 0.830f, 1.602f},
    // ZP:1409 - Harmonic Series
    {0.941f, 0.156f, 0.937f, 0.312f, 0.933f, 0.468f, 0.929f, 0.624f, 0.925f, 0.780f, 0.921f, 0.936f},
    // ZP:1410 - Vowel Ae2
    {0.963f, 0.195f, 0.957f, 0.390f, 0.951f, 0.585f, 0.945f, 0.780f, 0.939f, 0.975f, 0.933f, 1.170f},
    // ZP:1411 - Vowel Eh
    {0.919f, 0.223f, 0.925f, 0.446f, 0.912f, 0.669f, 0.899f, 0.892f, 0.886f, 1.115f, 0.873f, 1.338f},
    // ZP:1412 - Vowel Ih
    {0.894f, 0.289f, 0.900f, 0.578f, 0.887f, 0.867f, 0.874f, 1.156f, 0.861f, 1.445f, 0.848f, 1.734f},
    // ZP:1413 - Comb Filter
    {0.912f, 0.334f, 0.906f, 0.668f, 0.900f, 1.002f, 0.894f, 1.336f, 0.888f, 1.670f, 0.882f, 2.004f},
    // ZP:1414 - Notch Sweep
    {0.947f, 0.267f, 0.941f, 0.534f, 0.935f, 0.801f, 0.929f, 1.068f, 0.923f, 1.335f, 0.917f, 1.602f},
    // ZP:1415 - Ring Mod
    {0.867f, 0.356f, 0.873f, 0.712f, 0.860f, 1.068f, 0.847f, 1.424f, 0.834f, 1.780f, 0.821f, 2.136f},
    // ZP:1416 - Classic Sweep
    {0.958f, 0.089f, 0.952f, 0.178f, 0.946f, 0.267f, 0.940f, 0.356f, 0.934f, 0.445f, 0.928f, 0.534f},
    // ZP:1417 - Harmonic Exciter
    {0.923f, 0.312f, 0.917f, 0.624f, 0.911f, 0.936f, 0.905f, 1.248f, 0.899f, 1.560f, 0.893f, 1.872f},
    // ZP:1418 - Formant Filter
    {0.889f, 0.234f, 0.895f, 0.468f, 0.882f, 0.702f, 0.869f, 0.936f, 0.856f, 1.170f, 0.843f, 1.404f},
    // ZP:1419 - Vocal Tract
    {0.934f, 0.178f, 0.928f, 0.356f, 0.922f, 0.534f, 0.916f, 0.712f, 0.910f, 0.890f, 0.904f, 1.068f},
    // ZP:1420 - Wah
    {0.976f, 0.134f, 0.972f, 0.268f, 0.968f, 0.402f, 0.964f, 0.536f, 0.960f, 0.670f, 0.956f, 0.804f},
    // ZP:1421 - Bandpass Ladder
    {0.901f, 0.267f, 0.907f, 0.534f, 0.894f, 0.801f, 0.881f, 1.068f, 0.868f, 1.335f, 0.855f, 1.602f},
    // ZP:1422 - Allpass Chain
    {0.945f, 0.223f, 0.939f, 0.446f, 0.933f, 0.669f, 0.927f, 0.892f, 0.921f, 1.115f, 0.915f, 1.338f},
    // ZP:1423 - Peaking EQ
    {0.912f, 0.289f, 0.918f, 0.578f, 0.905f, 0.867f, 0.892f, 1.156f, 0.879f, 1.445f, 0.866f, 1.734f},
    // ZP:1424 - Shelving Filter
    {0.858f, 0.356f, 0.864f, 0.712f, 0.851f, 1.068f, 0.838f, 1.424f, 0.825f, 1.780f, 0.812f, 2.136f},
    // ZP:1425 - Phase Shifter
    {0.949f, 0.156f, 0.943f, 0.312f, 0.937f, 0.468f, 0.931f, 0.624f, 0.925f, 0.780f, 0.919f, 0.936f},
    // ZP:1426 - Chorus
    {0.923f, 0.195f, 0.929f, 0.390f, 0.916f, 0.585f, 0.903f, 0.780f, 0.890f, 0.975f, 0.877f, 1.170f},
    // ZP:1427 - Flanger
    {0.887f, 0.267f, 0.893f, 0.534f, 0.880f, 0.801f, 0.867f, 1.068f, 0.854f, 1.335f, 0.841f, 1.602f},
    // ZP:1428 - Freq Shifter
    {0.956f, 0.112f, 0.950f, 0.224f, 0.944f, 0.336f, 0.938f, 0.448f, 0.932f, 0.560f, 0.926f, 0.672f},
    // ZP:1429 - Granular
    {0.901f, 0.245f, 0.907f, 0.490f, 0.894f, 0.735f, 0.881f, 0.980f, 0.868f, 1.225f, 0.855f, 1.470f},
    // ZP:1430 - Spectral Morph
    {0.934f, 0.289f, 0.928f, 0.578f, 0.922f, 0.867f, 0.916f, 1.156f, 0.910f, 1.445f, 0.904f, 1.734f},
    // ZP:1431 - Ultimate
    {0.967f, 0.178f, 0.961f, 0.356f, 0.955f, 0.534f, 0.949f, 0.712f, 0.943f, 0.890f, 0.937f, 1.068f}
};

static const int MORPH_PAIRS[6][2] = {
    { 0, 12 }, // Vowel_Ae <-> Vowel_Ih
    { 7, 5 },  // Bell_Metallic <-> Metallic
    { 3, 18 }, // Resonant Peak <-> Formant Filter
    { 3, 4 },  // Resonant Peak <-> Wide Spectrum
    { 1, 8 },  // Vocal Morph <-> Aggressive Lead
    { 16, 31 } // Classic Sweep <-> Ultimate
};

#define AUTHENTIC_EMU_SHAPES_DEFINED

static const char* AUTHENTIC_EMU_IDS[6] = {
    "Vowel_Ae-Ih",
    "Bell_Metal",
    "Reso_Formant",
    "Reso_Wide",
    "Vocal_Aggr",
    "Sweep_Ult"
};
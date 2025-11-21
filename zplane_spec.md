Role: Expert Audio DSP & Graphics Engineer (C++ / JUCE)

Objective:
Build the source code for "HyperCube," a high-end VST3/AU filter plugin that emulates the E-mu Z-Plane "H-Chip" hardware. The project must prioritize authentic "vintage" DSP behavior (integer saturation) and a "Lexicon-style" 3D visualization.

Input Context:
You are provided with two core files that must be used without modification:

emu_zplane_core.cpp (The DSP Engine)

zplane_visualizer_math.cpp (The 3D Geometry Helper)

Output Requirements:
Generate the following 4 files to replace the default files in a standard JUCE CMake project:

PluginProcessor.h

PluginProcessor.cpp

PluginEditor.h

PluginEditor.cpp

1. DSP & Architecture (The "Engine")

Core Logic: Must instantiate EmuHChipFilter and ZPlaneCube from the input files.

Parameters (APVTS):

Morph (0.0 - 1.0): Controls the Z-Plane interpolation X-axis.

Frequency (0.0 - 1.0): Controls the Y-axis.

Transform (0.0 - 1.0): Controls the Z-axis.

Grit (Bool): "Vintage Mode". When enabled, forces the DSP loop to use saturate() (int32 clipping). When disabled, uses standard float headroom.

Audio Loop:

Convert float buffer -> fixed_to_float -> process_biquad -> float output.

Must be stereo (process Left and Right channels independently).

2. Visuals & Atmosphere (The "Glass Cube")

Renderer: Use juce::OpenGLContext attached to the Editor component for 60FPS performance.

Aesthetic: "90s High-End DSP" (Lexicon PCM-90 vibe).

Background: Deep, dark slate blue gradient.

Grid: A perspective wireframe floor in Cyan/Teal.

Ribbons: Use ZPlaneVisualizer::generate_ribbons to draw the 3D frequency response curves.

Active Curve: Bright Yellow, thick line width.

History/Future Curves: Translucent Blue/Purple, fading into the distance.

Interaction:

The Puck: A glowing 3D orb floating inside the visualizer.

Control: User drags the Puck to change Morph (Depth) and Frequency (Left/Right).

3. Tech Constraints

Threading: DSP runs on Audio Thread, OpenGL runs on Message/Rendering Thread. Use std::atomic or standard JUCE parameter listeners to sync them safely.

No External Assets: Do not require external PNGs. Draw all knobs and UI elements using juce::Graphics vector drawing or OpenGL primitives.

Performance: The visualizer must not block the audio thread.

Command:
Generate the authentic C++ code for the 4 required files now.
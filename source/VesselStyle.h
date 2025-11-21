// VesselStyle.h
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace VesselStyle {
    
    // PALETTE: "The Rack Unit" (Lexicon/Eventide 90s Era)
    // Background: Industrial powder-coated metal.
    static const juce::Colour Color_Paper       = juce::Colour(0xFF181818); 
    
    // Ink: Silk-screened panel text (Off-White).
    static const juce::Colour Color_Ink         = juce::Colour(0xFFD0D0D0); 
    
    // Grid: Faint guide lines, barely visible on the black metal.
    static const juce::Colour Color_Grid        = juce::Colour(0xFF303030);
    
    // Action: LED Red for overs, clips, and active states.
    static const juce::Colour Color_Active      = juce::Colour(0xFFFF3030); 
    
    // Structure: Bezel/Frame grey.
    static const juce::Colour Color_Structure   = juce::Colour(0xFF404040);
    
    // Display: VFD (Vacuum Fluorescent Display) Cyan
    static const juce::Colour Color_VFD         = juce::Colour(0xFF00F0FF);

    // DIMENSIONS
    static const float GridSize = 20.0f; 
    static const float LineThickness = 1.0f;

    // TYPOGRAPHY
    static juce::Font getMonospaceFont(float height) {
        return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
    }
}
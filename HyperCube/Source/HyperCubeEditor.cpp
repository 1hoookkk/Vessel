#include "HyperCubeEditor.h"

HyperCubeEditor::HyperCubeEditor(HyperCubeProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Setup OpenGL
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(true); // 60FPS VSync usually

    // 2. Setup UI Controls (Hidden or Minimal)
    // We'll make them visible for fallback/debug, but the main interaction is the Puck
    addAndMakeVisible(morphSlider);
    morphAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "morph", morphSlider);
    morphSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    morphSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    morphSlider.setAlpha(0.0f); // Hidden interaction

    addAndMakeVisible(freqSlider);
    freqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "freq", freqSlider);
    freqSlider.setAlpha(0.0f);

    addAndMakeVisible(transformSlider);
    transAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "transform", transformSlider);
    
    addAndMakeVisible(gritButton);
    gritAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "grit", gritButton);
    gritButton.setButtonText("VINTAGE GRIT");
    
    // 3. Pre-calculate static grid
    gridLines = ZPlaneVisualizer::generate_perspective_grid();

    setSize(800, 600);
    startTimerHz(60); // Update logic at 60Hz
}

HyperCubeEditor::~HyperCubeEditor()
{
    stopTimer();
    openGLContext.detach();
}

void HyperCubeEditor::timerCallback()
{
    // Fetch current parameters to drive animation
    // In a real app we'd use an Atomic or Listener, here we poll
    currentMorph = *audioProcessor.apvts.getRawParameterValue("morph");
    currentFreq = *audioProcessor.apvts.getRawParameterValue("freq");
    float trans = *audioProcessor.apvts.getRawParameterValue("transform");

    // Regenerate Ribbons based on current state
    // Note: Doing this on Message Thread is fine for this complexity (~1000 points)
    ribbons = ZPlaneVisualizer::generate_ribbons(currentFreq, trans, audioProcessor.getSampleRate());
}

void HyperCubeEditor::paint(juce::Graphics& g)
{
    // If OpenGL is active, this might be drawn *over* the GL context or unused.
    // We'll draw the controls overlay here.
    
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("HYPERCUBE // EMU H-CHIP EMULATION", 20, 20, 300, 20, juce::Justification::left);
    
    // Draw "Puck" Logic indicator if needed, but we do it in GL for 3D depth
}

void HyperCubeEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(40);
    auto footer = area.removeFromBottom(40);
    
    gritButton.setBounds(footer.removeFromRight(100));
    transformSlider.setBounds(footer); // Bottom slider for Transform
    
    // Morph/Freq are X/Y dragged, so sliders are hidden
    morphSlider.setBounds(0,0,0,0);
    freqSlider.setBounds(0,0,0,0);
}

void HyperCubeEditor::mouseDown(const juce::MouseEvent& e)
{
    // Hit test for the "Cube" area
    mouseDrag(e);
}

void HyperCubeEditor::mouseDrag(const juce::MouseEvent& e)
{
    // Map X/Y to Morph/Freq
    float xNorm = (float)e.x / (float)getWidth();
    float yNorm = 1.0f - (float)e.y / (float)getHeight(); // Y is up
    
    // Update Parameters via Attachments (or direct if needed)
    morphSlider.setValue(xNorm);
    freqSlider.setValue(yNorm);
}

// =========================================================
// OpenGL Renderer
// =========================================================

void HyperCubeEditor::newOpenGLContextCreated()
{
    // Setup Shaders if we were using them. 
    // For this prototype, we will use immediate mode or helper functions 
    // (deprecated but simple for "spec" generation) or JUCE's LowLevelGraphicsContext 
    // if we wanted 2D-in-3D. 
    // Given constraints, I'll stick to standard GL calls assuming <gl/gl.h> provided by context.
}

void HyperCubeEditor::openGLContextClosing()
{
}

void HyperCubeEditor::renderOpenGL()
{
    // 1. Background (Deep Slate Blue)
    using namespace juce::gl;
    
    // Helper to clear
    // OpenGLContext automatically sets up the viewport
    
    // Clear Color: Dark Slate Blue #1a1a2e approx
    glClearColor(0.1f, 0.1f, 0.18f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 2. Setup 3D Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Simple perspective
    float aspect = (float)getWidth() / (float)getHeight();
    // glFrustum or similar. Let's fake it with simple coordinate mapping for the ribbon logic
    // The ribbon logic output 0..1 coordinates. We can map them to screen space -1..1
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw Grid (Cyan/Teal wireframe floor)
    // We interpret Z in VisPoint3D as depth into screen.
    // X as left-right (-1 to 1)
    // Y as up-down (-1 to 1)
    
    auto drawLine = [](const VisPoint3D& p1, const VisPoint3D& p2, float alpha) {
        // Perspective transform simulation
        // Simple: x' = x / (z + dist)
        float dist = 1.5f;
        float z1 = p1.z * 0.8f + 0.2f; // push back
        float z2 = p2.z * 0.8f + 0.2f;
        
        float x1 = (p1.x * 2.0f - 1.0f); // 0..1 -> -1..1
        float y1 = (p1.y * 2.0f - 1.0f);
        
        float x2 = (p2.x * 2.0f - 1.0f);
        float y2 = (p2.y * 2.0f - 1.0f);
        
        // Apply depth scale
        float scale1 = 1.0f / (z1 + 0.5f);
        float scale2 = 1.0f / (z2 + 0.5f);
        
        glVertex3f(x1 * scale1, y1 * scale1 - 0.5f, -z1); // Shift Y down for "floor" look if grid
        glVertex3f(x2 * scale2, y2 * scale2 - 0.5f, -z2);
    };
    
    // Draw Floor Grid
    glLineWidth(1.0f);
    glColor4f(0.0f, 1.0f, 1.0f, 0.2f); // Cyan
    glBegin(GL_LINES);
    for(auto& line : gridLines) {
        if(line.size() >= 2) {
            // Flat floor: set Y to -1.0 (visually)
            VisPoint3D p1 = line[0]; p1.y = 0.0f;
            VisPoint3D p2 = line[1]; p2.y = 0.0f;
            // Adjust coordinate system: Input grid is 0..1.
            // We want floor at bottom of screen.
            // Let's map input Y (0) to -1.0 screen Y.
            
            // Actually, let's just use the helper logic above but offset Y
            // Override Y for grid to be "floor"
            // The drawLine helper is tuned for the ribbons. 
            // Let's write raw GL for simplicity.
            
            // 3D projection is better handled by gluPerspective but strictly we don't have GLU guaranteed.
            // We'll just draw 2D lines that look 3D (Isometric-ish)
            
            float d1 = 1.0f + p1.z;
            float d2 = 1.0f + p2.z;
            
            float sx1 = (p1.x * 2.0f - 1.0f) / d1;
            float sy1 = -0.8f / d1; // Fixed floor height
            
            float sx2 = (p2.x * 2.0f - 1.0f) / d2;
            float sy2 = -0.8f / d2;
            
            glVertex2f(sx1, sy1);
            glVertex2f(sx2, sy2);
        }
    }
    glEnd();

    // 3. Draw Ribbons
    // Active Curve (History 0) -> Yellow
    // Others -> Blue/Purple fading
    
    for (int i = 0; i < ribbons.size(); ++i) {
        const auto& ribbon = ribbons[i];
        if (ribbon.empty()) continue;
        
        float depth = (float)i / (float)ribbons.size(); // 0 is front
        bool isActive = (i == 0); // Assuming 0 is front/active
        
        if (isActive) {
            glLineWidth(3.0f);
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        } else {
            glLineWidth(1.0f);
            glColor4f(0.3f, 0.0f, 1.0f, 0.5f * (1.0f - depth)); // Purple fade
        }
        
        glBegin(GL_LINE_STRIP);
        for (const auto& p : ribbon) {
            // Perspective Projection Math
            float z = p.z; // 0..1
            float dist = 1.0f + z * 1.5f; // Depth scale
            
            float x = (p.x * 2.0f - 1.0f) * 0.9f; // Margin
            float y = (p.y * 2.0f - 1.0f) * 0.5f; // Scale height
            
            // Apply projection
            float sx = x / dist;
            float sy = y / dist; // Center vertically
            
            // Shift up slightly to sit on floor
            sy += (-0.3f / dist); 
            
            glVertex2f(sx, sy);
        }
        glEnd();
    }
    
    // 4. Draw Puck (The glowing orb)
    // Position based on current parameters
    float puckX = (currentMorph * 2.0f - 1.0f) / 1.0f; // Morph is X (or Depth?)
    // Spec says: "User drags Puck to change Morph (Depth) and Frequency (Left/Right)."
    // Wait, usually Morph is Z-depth in Z-Plane. Frequency is X.
    // Let's map:
    // X-axis: Frequency (currentFreq)
    // Z-axis (Depth): Morph (currentMorph)
    
    float pZ = currentMorph;
    float pX = currentFreq;
    float pDist = 1.0f + pZ * 1.5f;
    
    float screenX = (pX * 2.0f - 1.0f) * 0.9f / pDist;
    float screenY = (-0.3f / pDist); // On the floor
    
    glPointSize(15.0f);
    glBegin(GL_POINTS);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f); // Core
    glVertex2f(screenX, screenY);
    glEnd();
    
    glPointSize(25.0f);
    glBegin(GL_POINTS);
    glColor4f(0.0f, 1.0f, 1.0f, 0.4f); // Glow
    glVertex2f(screenX, screenY);
    glEnd();
}

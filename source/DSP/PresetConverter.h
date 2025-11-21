#pragma once

#include "EmuZPlaneCore.h"
#include "EMUAuthenticTables.h"
#include <vector>
#include <cmath>
#include <algorithm>

/**
 * Phase 1.2: Preset Converter
 * Maps legacy EMUAuthenticTables (Pole/Zero data) to ZPlaneCube (k-space data).
 */
class PresetConverter {
public:
    // Converts a pair of legacy shape indices (A and B) into a ZPlaneCube.
    // The Cube will morph from Shape A (at Morph=0) to Shape B (at Morph=1).
    // The Y and Z axes will be identical to the X axis (1D morph only for now).
    static ZPlaneCube convertToCube(int shapeIndexA, int shapeIndexB);

private:
    static ZPlaneFrame convertShapeToFrame(int shapeIndex);
    static ZPlaneSection convertPoleToSection(float r, float theta);
};

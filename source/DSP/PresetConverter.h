/**
 * Preset Converter - Phase 1.2 Deliverable
 * Maps EMUAuthenticTables (12-coefficient format) to ZPlaneCube (k-space format)
 *
 * The EMU shapes are stored as 6 pairs of (pitch, resonance) coefficients per shape.
 * This converter transforms them into a 3D ZPlaneCube with 8 corners for morphing.
 */

#pragma once

#include "EmuZPlaneCore.h"
#include "EMUAuthenticTables.h"
#include <array>

class PresetConverter {
public:
    /**
     * Convert two EMU shape indices into a ZPlaneCube
     * The cube corners are populated for smooth morphing between shapeA and shapeB
     *
     * @param shapeIndexA Index into AUTHENTIC_EMU_SHAPES (0-31)
     * @param shapeIndexB Index into AUTHENTIC_EMU_SHAPES (0-31)
     * @return ZPlaneCube with 8 corners for trilinear interpolation
     */
    static ZPlaneCube convertToCube(int shapeIndexA, int shapeIndexB);

private:
    static ZPlaneFrame convertShapeToFrame(int shapeIndex);
    static ZPlaneSection convertPoleToSection(float r, float theta);
};

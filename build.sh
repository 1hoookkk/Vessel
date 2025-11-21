#!/bin/bash

# VESSEL Build Script
# Project ARMAdillo

set -e  # Exit on error

echo "========================================="
echo "  VESSEL - Z-Plane Filter Plugin"
echo "  Project ARMAdillo Build System"
echo "========================================="
echo ""

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install CMake 3.24 or higher."
    exit 1
fi

# Build type (default: Release)
BUILD_TYPE="${1:-Release}"

echo "Build Type: $BUILD_TYPE"
echo ""

# Create build directory
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Configure CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
echo ""
echo "Building VESSEL..."
cmake --build . --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "========================================="
echo "  Build Complete!"
echo "========================================="
echo ""
echo "Plugin artifacts:"
echo "  VST3: $BUILD_DIR/Vessel_artefacts/$BUILD_TYPE/VST3/"
echo "  AU:   $BUILD_DIR/Vessel_artefacts/$BUILD_TYPE/AU/"
echo ""
echo "To clean build: rm -rf $BUILD_DIR"
echo ""

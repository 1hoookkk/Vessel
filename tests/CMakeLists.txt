# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Consolidated Test Executable (Optimized Build)
add_executable(Heavy_Tests
    unit/Test_EMUStaticShapeBank.cpp
    unit/Test_WaitFreeTripleBuffer.cpp
    unit/Test_PresetConverter.cpp
    integration/Test_VesselProcessor.cpp
)

target_link_libraries(Heavy_Tests PRIVATE Catch2::Catch2WithMain Heavy_Core)
target_include_directories(Heavy_Tests PRIVATE ${CMAKE_SOURCE_DIR}/source)
target_compile_features(Heavy_Tests PRIVATE cxx_std_20)

# Allow access to internal headers
target_include_directories(Heavy_Tests PRIVATE ${CMAKE_SOURCE_DIR}/source/DSP)

# Discover tests for CTest
include(Catch)
catch_discover_tests(Heavy_Tests)

# Z-Plane Core Standalone Verification Test
add_executable(ZPlane_Core_Test
    zplane_core_test.cpp
    ../source/DSP/EmuZPlaneCore.cpp
    ../source/DSP/PresetConverter.cpp
)

target_include_directories(ZPlane_Core_Test PRIVATE ${CMAKE_SOURCE_DIR}/source/DSP)

target_compile_features(ZPlane_Core_Test PRIVATE cxx_std_20)

# Z-Plane Visualizer Wiring Test (Phase 3.1 Prototype)
add_executable(Visualizer_Integration_Test
    visualiser_integration_test.cpp
    ../source/DSP/EmuZPlaneCore.cpp
)
target_include_directories(Visualizer_Integration_Test PRIVATE ${CMAKE_SOURCE_DIR}/source/DSP)
target_compile_features(Visualizer_Integration_Test PRIVATE cxx_std_20)

# Grey Box Test - The Filter Event Horizon (Foundation Test)
add_executable(Grey_Box_Test
    grey_box_test.cpp
    ../source/DSP/EmuZPlaneCore.cpp
    ../source/DSP/PresetConverter.cpp
)
target_include_directories(Grey_Box_Test PRIVATE ${CMAKE_SOURCE_DIR}/source/DSP)
target_compile_features(Grey_Box_Test PRIVATE cxx_std_20)

# Grey Box Diagnostic - Debug version with coefficient output
add_executable(Grey_Box_Diagnostic
    grey_box_diagnostic.cpp
    ../source/DSP/EmuZPlaneCore.cpp
    ../source/DSP/PresetConverter.cpp
)
target_include_directories(Grey_Box_Diagnostic PRIVATE ${CMAKE_SOURCE_DIR}/source/DSP)
target_compile_features(Grey_Box_Diagnostic PRIVATE cxx_std_20)

# Add to CTest
add_test(NAME ZPlaneCoreVerification COMMAND ZPlane_Core_Test)
add_test(NAME VisualizerWiringVerification COMMAND Visualizer_Integration_Test)
add_test(NAME GreyBoxFilterEventHorizon COMMAND Grey_Box_Test)

<#
    VESSEL // System_A01 // Build Sequence
    Usage: .\build.ps1
#>

$BuildDir = "build"
$Generator = "Visual Studio 17 2022" # Standard for modern Windows dev
$Config = "Release"

Write-Host ":: VESSEL // INITIALIZING BUILD SYSTEM..." -ForegroundColor Cyan

# 1. Check for CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake is not found in your PATH. Please install CMake."
    exit 1
}

# 2. Configure
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

Write-Host ":: CONFIGURING (Fetching JUCE - This might take a minute)..." -ForegroundColor Yellow
cmake -S . -B $BuildDir -G $Generator
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# 3. Build
Write-Host ":: COMPILING..." -ForegroundColor Yellow
cmake --build $BuildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ":: BUILD COMPLETE." -ForegroundColor Green
Write-Host ":: Artifacts located in: $BuildDir/Vessel_artefacts/$Config" -ForegroundColor Gray

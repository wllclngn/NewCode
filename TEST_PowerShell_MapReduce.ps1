# Test Script for MapReduce Project

Write-Host "Starting final testing..."

# Step 1: Clean and Rebuild
Write-Host "Cleaning and rebuilding the project..."
if (Test-Path -Path "build") {
    Remove-Item -Recurse -Force "build"
}
New-Item -ItemType Directory -Path "build" | Out-Null
Set-Location -Path "build"
cmake .. | Out-Host
if (-not $?) {
    Write-Error "CMake configuration failed."
    exit 1
}
make | Out-Host
if (-not $?) {
    Write-Error "Build failed."
    exit 1
}

# Step 2: Run Tests
Write-Host "Running tests..."
if (-not (./utils_test)) {
    Write-Error "Utils tests failed."
    exit 1
}
if (-not (./mapper_test)) {
    Write-Error "Mapper tests failed."
    exit 1
}
if (-not (./reducer_test)) {
    Write-Error "Reducer tests failed."
    exit 1
}
if (-not (./integration_test)) {
    Write-Error "Integration tests failed."
    exit 1
}

# Step 3: Manual Test with Sample Data
Write-Host "Running manual test..."
Set-Location -Path ".."
if (-not (Test-Path -Path "sample_input")) {
    New-Item -ItemType Directory -Path "sample_input" | Out-Null
}
if (-not (Test-Path -Path "sample_output")) {
    New-Item -ItemType Directory -Path "sample_output" | Out-Null
}
if (-not (Test-Path -Path "sample_temp")) {
    New-Item -ItemType Directory -Path "sample_temp" | Out-Null
}

Set-Content -Path "sample_input/file1.txt" -Value "Hello world`nHello again"
Set-Content -Path "sample_input/file2.txt" -Value "Another test`nHello world"

& .\build\MapReduce | Out-Host
if (-not $?) {
    Write-Error "Manual test failed."
    exit 1
}

# Step 4: Verify Output Files
Write-Host "Verifying output files..."
if ((Test-Path -Path "sample_output/output.txt") -and (Test-Path -Path "sample_output/output_summed.txt")) {
    Write-Host "Output files generated successfully."
} else {
    Write-Error "Output files missing!"
    exit 1
}

Write-Host "All tests passed. Project is ready for release!"

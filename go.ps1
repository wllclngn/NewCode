# PowerShell Script to Compile the MapReduce Project Using g++

Write-Host "Initiating project build..." -ForegroundColor Cyan

# Ensure g++ is available
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: g++ compiler not found. Please install g++ and try again." -ForegroundColor Red
    exit 1
}

# Define the source files and output binary
$sourceFiles = "main.cpp mapper.cpp reducer.cpp utils.cpp logger.cpp error_handler.cpp"
$outputBinary = "mapReduce.exe"

# Clean up any previous builds
if (Test-Path $outputBinary) {
    Write-Host "Cleaning up previous build..." -ForegroundColor Yellow
    Remove-Item $outputBinary
}

# Compile the project
Write-Host "Compiling source files..." -ForegroundColor Cyan
$compileCommand = "g++ -std=c++17 -o $outputBinary $sourceFiles -pthread"
Write-Host "Executing: $compileCommand" -ForegroundColor Green
Invoke-Expression $compileCommand

# Check if the compilation was successful
if ($LASTEXITCODE -eq 0) {
    Write-Host "SUCCESS: Build completed. Run the program with .\$outputBinary" -ForegroundColor Green
} else {
    Write-Host "ERROR: Build failed. Please check the error messages above for details." -ForegroundColor Red
    exit 1
}
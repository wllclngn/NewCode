# PowerShell Script to Compile the MapReduce Project Using MSVC, g++, or CMake with Pure PowerShell Methods

# Function to check if a command exists (e.g., cl, g++, cmake)
function Is-CommandAvailable {
    param (
        [string]$Command
    )
    try {
        $null = & where.exe $Command 2>$null
        return $true
    } catch {
        return $false
    }
}

# Function to clean up old files
function Clean-File {
    param (
        [string]$FilePath
    )
    if (Test-Path -Path $FilePath) {
        Remove-Item -Path $FilePath -Force
    }
}

# Function to execute a shell command
function Execute-Command {
    param (
        [string]$Command
    )
    Write-Host "Executing command: $Command" -ForegroundColor Green
    $process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c $Command" -Wait -NoNewWindow -PassThru
    $exitCode = $process.ExitCode
    if ($exitCode -ne 0) {
        Write-Host "ERROR: Command failed with exit code $exitCode" -ForegroundColor Red
    } else {
        Write-Host "Command executed successfully." -ForegroundColor Green
    }
    return $exitCode
}

# Function to dynamically set up MSVC environment for x64
function Setup-MSVCEnvironment {
    $vcvarsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $vcvarsPath) {
        Write-Host "Setting up MSVC environment for x64..."
        & cmd /c `"$vcvarsPath`"
    } else {
        Write-Host "ERROR: Cannot find vcvars64.bat. Ensure Visual Studio is installed and configured for x64." -ForegroundColor Red
        exit 1
    }
}

# Function to locate and set all necessary library and include paths
function Setup-LibraryAndIncludePaths {
    Write-Host "Using PowerShell to locate all library and include paths..." -ForegroundColor Cyan

    # Define library search paths (x64 only)
    $libSearchPaths = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.43.34808\lib\x64",
        "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64",
        "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"
    )

    # Define include search paths
    $includeSearchPaths = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.43.34808\include",
        "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt",
        "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um",
        "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared"
    )

    # Reset LIB and INCLUDE environment variables to avoid conflicts
    $env:LIB = ""
    $env:INCLUDE = ""

    # Update LIB environment variable
    foreach ($libPath in $libSearchPaths) {
        if ($env:LIB -notlike "*$libPath*") {
            $env:LIB = "$libPath;$env:LIB"
        }
    }

    # Update INCLUDE environment variable
    foreach ($includePath in $includeSearchPaths) {
        if ($env:INCLUDE -notlike "*$includePath*") {
            $env:INCLUDE = "$includePath;$env:INCLUDE"
        }
    }

    Write-Host "Updated LIB paths:" -ForegroundColor Yellow
    Write-Host $env:LIB
    Write-Host "Updated INCLUDE paths:" -ForegroundColor Yellow
    Write-Host $env:INCLUDE
}

# Start the build process
Write-Host "Starting the build process..." -ForegroundColor Cyan

# Set up MSVC environment for x64
Setup-MSVCEnvironment

# Set up library and include paths
Setup-LibraryAndIncludePaths

# Check for available build tools
$useMSVC = Is-CommandAvailable -Command "cl.exe"
$useGpp = Is-CommandAvailable -Command "g++"
$useCMake = Is-CommandAvailable -Command "cmake"

if (-not ($useMSVC -or $useGpp -or $useCMake)) {
    Write-Host "ERROR: No compatible build tools found (MSVC, g++, or CMake). Please install one and try again." -ForegroundColor Red
    exit 1
}

# Define source files and output binary
$sourceFiles = "main.cpp"
$outputBinary = "MapReduce.exe"
$outputDLL = "LibMapReduce.dll"

# Clean up previous builds
Write-Host "Cleaning up previous builds..." -ForegroundColor Yellow
Clean-File -FilePath $outputBinary
Clean-File -FilePath $outputDLL

# Build using MSVC
if ($useMSVC) {
    Write-Host "Using MSVC (cl.exe) to build the project..." -ForegroundColor Cyan
    $msvcSharedLibraryCommand = "cl /EHsc /std:c++17 /MT /LD $sourceFiles /Fe:$outputDLL"
    if ((Execute-Command -Command $msvcSharedLibraryCommand) -ne 0) {
        Write-Host "ERROR: Failed to compile the shared library using MSVC. Exiting." -ForegroundColor Red
        exit 1
    }

    $msvcBinaryCommand = "cl /EHsc /std:c++17 /MT $sourceFiles /Fe:$outputBinary"
    if ((Execute-Command -Command $msvcBinaryCommand) -ne 0) {
        Write-Host "ERROR: Failed to compile the executable binary using MSVC. Exiting." -ForegroundColor Red
        exit 1
    }
}

# Build using g++
elseif ($useGpp) {
    Write-Host "Using g++ to build the project..." -ForegroundColor Cyan
    $gppSharedLibraryCommand = "g++ -std=c++17 -shared -fPIC -o $outputDLL $sourceFiles -pthread"
    if ((Execute-Command -Command $gppSharedLibraryCommand) -ne 0) {
        Write-Host "ERROR: Failed to compile the shared library using g++. Exiting." -ForegroundColor Red
        exit 1
    }

    $gppBinaryCommand = "g++ -std=c++17 -o $outputBinary $sourceFiles -pthread"
    if ((Execute-Command -Command $gppBinaryCommand) -ne 0) {
        Write-Host "ERROR: Failed to compile the executable binary using g++. Exiting." -ForegroundColor Red
        exit 1
    }
}

# Build using CMake
elseif ($useCMake) {
    Write-Host "Using CMake to build the project..." -ForegroundColor Cyan

    # Generate build system
    $cmakeGenerateCommand = "cmake -S . -B build"
    if ((Execute-Command -Command $cmakeGenerateCommand) -ne 0) {
        Write-Host "ERROR: Failed to generate build system using CMake. Exiting." -ForegroundColor Red
        exit 1
    }

    # Build the project
    $cmakeBuildCommand = "cmake --build build"
    if ((Execute-Command -Command $cmakeBuildCommand) -ne 0) {
        Write-Host "ERROR: Failed to build the project using CMake. Exiting." -ForegroundColor Red
        exit 1
    }
}

# Build completed successfully
Write-Host "Build process completed successfully!" -ForegroundColor Green
if ($useMSVC -or $useGpp) {
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "  - Shared Library: .\$outputDLL" -ForegroundColor Cyan
} else {
    Write-Host "  - Build artifacts are located in the 'build' directory." -ForegroundColor Cyan
}
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
    $process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c $Command" -Wait -NoNewWindow -PassThru -RedirectStandardOutput "stdout.log" -RedirectStandardError "stderr.log"
    $exitCode = $process.ExitCode
    if ($exitCode -ne 0) {
        Write-Host "ERROR: Command failed with exit code $exitCode" -ForegroundColor Red
        Write-Host "Error Output:" -ForegroundColor Yellow
        Get-Content "stderr.log" | Write-Host
    } else {
        Write-Host "Command executed successfully." -ForegroundColor Green
    }
    return $exitCode
}

# Start the build process
Write-Host "Starting the build process..." -ForegroundColor Cyan

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
$sharedLibrary = "LibMapReduce.dll"

# Clean up previous builds
Write-Host "Cleaning up previous builds..." -ForegroundColor Yellow
Clean-File -FilePath $outputBinary
Clean-File -FilePath $sharedLibrary

# Build using MSVC
if ($useMSVC) {
    Write-Host "Using MSVC (cl.exe) to build the project..." -ForegroundColor Cyan
    $msvcSharedLibraryCommand = "cl /EHsc /LD $sourceFiles /Fe:$sharedLibrary"
    if ((Execute-Command -Command $msvcSharedLibraryCommand) -ne 0) {
        Write-Host "ERROR: Failed to compile the shared library using MSVC. Exiting." -ForegroundColor Red
        exit 1
    }

    $msvcBinaryCommand = "cl /EHsc $sourceFiles /Fe:$outputBinary"
    if ((Execute-Command -Command $msvcBinaryCommand) -ne 0) {
        Write-Host "ERROR: Failed to compile the executable binary using MSVC. Exiting." -ForegroundColor Red
        exit 1
    }
}

# Build using g++
elseif ($useGpp) {
    Write-Host "Using g++ to build the project..." -ForegroundColor Cyan
    $gppSharedLibraryCommand = "g++ -std=c++17 -shared -fPIC -o $sharedLibrary $sourceFiles -pthread"
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
    Write-Host "  - Shared Library: .\$sharedLibrary" -ForegroundColor Cyan
} else {
    Write-Host "  - Build artifacts are located in the 'build' directory." -ForegroundColor Cyan
}
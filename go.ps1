# PowerShell Script to Compile the MapReduce Project Using MSVC, g++, or CMake with Pure PowerShell Methods

# Function to check if a command exists (e.g., cl, g++, cmake)
function Is-CommandAvailable {
    param (
        [string]$Command
    )
    try {
        $null = Get-Command $Command -ErrorAction SilentlyContinue
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
        Write-Host "Cleaning file: $FilePath"
        Remove-Item -Path $FilePath -Force
    }
}

# Function to execute a shell command
function Execute-Command {
    param (
        [string]$Command,
        [string]$CommandArgs
    )
    Write-Host "Executing command: $Command $CommandArgs" -ForegroundColor Green
    $process = Start-Process -FilePath $Command -ArgumentList $CommandArgs -Wait -NoNewWindow -PassThru
    $exitCode = $process.ExitCode
    if ($exitCode -ne 0) {
        Write-Host "ERROR: Command failed with exit code $exitCode" -ForegroundColor Red
    } else {
        Write-Host "Command executed successfully." -ForegroundColor Green
    }
    return $exitCode
}

# Prompt the user for custom DLL files
function Get-CustomDLLsToLink {
    $customDLLsToLink = @()
    $useCustomDLLs = Read-Host "Do you have custom EXTERNAL DLL/shared library files you'd like to link? (yes/no)"
    if ($useCustomDLLs -eq "yes") {
        while ($true) {
            $dllPath = Read-Host "Enter the full path to your DLL/shared library file (or press Enter to finish)"
            if ([string]::IsNullOrWhiteSpace($dllPath)) {
                break
            }

            if (Test-Path -Path $dllPath -PathType Leaf) {
                $customDLLsToLink += $dllPath
            } else {
                Write-Host "Invalid path or not a file: '$dllPath'. Please try again." -ForegroundColor Red
            }
        }
    }
    return $customDLLsToLink
}

# Function to dynamically set up MSVC environment for x64
# (This function might need adjustments based on the specific Visual Studio version and installation path)
function Setup-MSVCEnvironment {
    $vcvarsPathOptions = @(
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
    $vcvarsPath = $vcvarsPathOptions | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($vcvarsPath) {
        Write-Host "Setting up MSVC environment for x64 using $vcvarsPath..."
        # Invoking vcvars64.bat in the current PowerShell session is tricky.
        # A common way is to call it in cmd.exe and then run the cl.exe command within that same cmd.exe instance.
        # For simplicity here, we'll assume cl.exe is findable if vcvars64.bat has been run previously in a command prompt
        # or if the environment is already configured (e.g., by opening a Developer Command Prompt).
        # The Execute-Command function will call cl.exe directly.
        # More robust solution would involve capturing environment variables set by vcvars64.bat.
        Write-Host "Please ensure MSVC environment is configured (e.g., run this script from a Developer Command Prompt for VS)."
    } else {
        Write-Host "WARNING: Cannot find vcvars64.bat. MSVC compilation might fail if environment is not pre-configured." -ForegroundColor Yellow
        # Exiting here might be too strict if user has another way of setting up MSVC env.
    }
}


# Start the build process
Write-Host "Starting the build process..." -ForegroundColor Cyan

# Setup MSVC environment (call this early, though its main effect is for cl.exe calls)
# Setup-MSVCEnvironment # User should run from dev prompt for MSVC

# Define source files, output binary, and link custom DLLs
# Assuming source files are in 'src\' and headers in 'include\' or '.'
$dllSourceFiles = @("src\Mapper.cpp", "src\Reducer.cpp") # Add other .cpp files for DLL as needed
$controllerSourceFiles = "src\main.cpp"

$outputSharedLib = "LibMapReduce.dll" # For MSVC/g++ on Windows
$outputControllerBinary = "mapreduce_controller.exe"

# Adjust shared library name for g++ on non-Windows (though this script is .ps1, g++ might be used via WSL or MinGW)
if ($IsWindows -ne $true -or (Is-CommandAvailable -Command "g++")) {
    # If using g++ (MinGW or similar on Windows, or PowerShell Core on Linux/macOS)
    if ($IsWindows -ne $true) { # PowerShell on Linux/macOS
      $outputSharedLib = "LibMapReduce.so" # Or .dylib on macOS, g++ handles this
    }
}


$customLibsToLink = Get-CustomDLLsToLink
$customLinkArgs = $customLibsToLink -join " "


# Include paths
$includeArgs = "-I.", "-Iinclude", "-Isrc" # Common include paths

# Check for available build tools
$useMSVC = Is-CommandAvailable -Command "cl.exe"
$useGpp = Is-CommandAvailable -Command "g++"
$useCMake = Is-CommandAvailable -Command "cmake"

if (-not ($useMSVC -or $useGpp -or $useCMake)) {
    Write-Host "ERROR: No compatible build tools found (MSVC cl.exe, g++, or CMake). Please install one and try again." -ForegroundColor Red
    exit 1
}

# Clean up previous builds
Write-Host "Cleaning up previous builds..." -ForegroundColor Yellow
Clean-File -FilePath $outputSharedLib
Clean-File -FilePath $outputControllerBinary
Clean-File -FilePath ($outputControllerBinary -replace ".exe", ".exp") # MSVC specific
Clean-File -FilePath ($outputControllerBinary -replace ".exe", ".lib") # MSVC specific


# Build based on available tools
if ($useMSVC) {
    Write-Host "Using MSVC (cl.exe) to build the project..." -ForegroundColor Cyan
    # MSVC flags: /EHsc (exception handling), /std:c++17, /MD (multithreaded DLL runtime)
    # For DLL: /LD (create DLL)
    $msvcFlags = "/EHsc", "/std:c++17", "/MD", "/Wall" # /MD for linking with MSVCRT.dll
    $msvcIncludeArgs = $includeArgs -replace "-I", "/I"
    
    # Compile Shared Library (DLL)
    $msvcDllArgs = $msvcFlags + $msvcIncludeArgs + "/LD" + "/Fe:$outputSharedLib" + $dllSourceFiles
    if ((Execute-Command -Command "cl.exe" -CommandArgs $msvcDllArgs) -ne 0) {
        Write-Host "ERROR: Failed to compile the shared library using MSVC. Exiting." -ForegroundColor Red
        exit 1
    }

    # Compile Executable, linking the DLL
    # For MSVC, linking .dll involves linking its associated .lib file (import library)
    # The .lib file is generated when the DLL is built.
    $msvcExeArgs = $msvcFlags + $msvcIncludeArgs + "/Fe:$outputControllerBinary" + $controllerSourceFiles + "$($outputSharedLib -replace '.dll', '.lib')" + $customLibsToLink
    if ((Execute-Command -Command "cl.exe" -CommandArgs $msvcExeArgs) -ne 0) {
        Write-Host "ERROR: Failed to compile the executable binary using MSVC. Exiting." -ForegroundColor Red
        exit 1
    }
}
elseif ($useGpp) {
    Write-Host "Using g++ to build the project..." -ForegroundColor Cyan
    # g++ flags: -std=c++17, -pthread, -Wall, -Wextra, -O2
    $gppFlags = "-std=c++17", "-pthread", "-Wall", "-Wextra", "-O2" # Add -fPIC if not on Windows, though g++ on Win handles it.
    if ($IsWindows -ne $true) { $gppFlags += "-fPIC" }

    # Compile Shared Library
    $gppDllArgs = $gppFlags + "-shared" + "-o", $outputSharedLib + $dllSourceFiles + $includeArgs
    if ((Execute-Command -Command "g++" -CommandArgs $gppDllArgs) -ne 0) {
        Write-Host "ERROR: Failed to compile the shared library using g++. Exiting." -ForegroundColor Red
        exit 1
    }

    # Compile Executable, linking the DLL
    # For g++, link directly to the .so or .dll file.
    # -L. to search current dir, -l<name> for lib<name>.so, or direct path.
    # -Wl,-rpath,'$ORIGIN' or equivalent for non-Windows if applicable for PowerShell Core scenarios
    $gppExeArgs = $gppFlags + "-o", $outputControllerBinary + $controllerSourceFiles + $includeArgs + "./$outputSharedLib" + $customLibsToLink
    if ($IsWindows -ne $true) { # Add rpath for Linux/macOS if using PowerShell Core
        $gppExeArgs += "-Wl,-rpath,'\$ORIGIN'" # Escaped $ for PowerShell
    }
    if ((Execute-Command -Command "g++" -CommandArgs $gppExeArgs) -ne 0) {
        Write-Host "ERROR: Failed to compile the executable binary using g++. Exiting." -ForegroundColor Red
        exit 1
    }
}
elseif ($useCMake) {
    Write-Host "Using CMake to build the project..." -ForegroundColor Cyan
    Write-Host "Note: CMake build relies on a correctly configured CMakeLists.txt file." -ForegroundColor Yellow
    Write-Host "Ensure CMakeLists.txt defines 'LibMapReduce' shared library and 'mapreduce_controller' executable." -ForegroundColor Yellow

    if (-not (Test-Path -Path "CMakeLists.txt")) {
        Write-Host "ERROR: CMakeLists.txt not found in the current directory. CMake build cannot proceed." -ForegroundColor Red
        exit 1
    }
    
    # Create build directory if it doesn't exist
    if (-not (Test-Path -Path "build" -PathType Container)) {
        New-Item -ItemType Directory -Path "build" | Out-Null
    }

    # Configure CMake (adjust generator if needed, e.g., -G "Visual Studio 17 2022")
    $cmakeConfigureArgs = "-S", ".", "-B", "build" 
    if ((Execute-Command -Command "cmake" -CommandArgs $cmakeConfigureArgs) -ne 0) {
        Write-Host "ERROR: CMake configuration failed. Exiting." -ForegroundColor Red
        exit 1
    }

    # Build with CMake
    $cmakeBuildArgs = "--build", "build"
    if ((Execute-Command -Command "cmake" -CommandArgs $cmakeBuildArgs) -ne 0) {
        Write-Host "ERROR: CMake build failed. Exiting." -ForegroundColor Red
        exit 1
    }
    Write-Host "CMake build successful. Artifacts are in the 'build' directory (and potentially its subdirectories like build/Debug or build/Release)." -ForegroundColor Green
    Write-Host "Look for $outputSharedLib and $outputControllerBinary in the build output location."
}

# Build completed successfully (for MSVC/g++ path)
if ($useMSVC -or $useGpp) {
    Write-Host "Build process completed successfully!" -ForegroundColor Green
    Write-Host "  - Shared Library: .\$outputSharedLib" -ForegroundColor Cyan
    Write-Host "  - Controller Executable: .\$outputControllerBinary" -ForegroundColor Cyan
    Write-Host "You can run the controller with: .\$outputControllerBinary <input_dir> <output_dir> <temp_dir> <M> <R>"
}

if ($customLibsToLink.Count -gt 0) {
    Write-Host "Custom external libraries linked:" -ForegroundColor Yellow
    $customLibsToLink | ForEach-Object { Write-Host "  - $_" }
}
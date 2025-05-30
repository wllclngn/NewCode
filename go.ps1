# PowerShell Script to Compile the MapReduce Project
# Priority: 1. MSVC (cl.exe), 2. g++, 3. CMake

# Function to check if a command exists
function Is-CommandAvailable {
    param ([string]$Command)
    try { $null = & where.exe $Command 2>$null; return $true } catch { return $false }
}

# Function to clean up old files
function Clean-File {
    param ([string]$FilePath)
    if (Test-Path -Path $FilePath) { Remove-Item -Path $FilePath -Force }
}

# Function to execute a shell command
function Execute-Command {
    param ([string]$Command)
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
    $vcvarsPathX86 = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    $vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    $selectedPath = ""

    if (Test-Path $vcvarsPath) {
        $selectedPath = $vcvarsPath
    } elseif (Test-Path $vcvarsPathX86) {
        $selectedPath = $vcvarsPathX86
    }

    if ($selectedPath) {
        Write-Host "Setting up MSVC environment for x64 using $selectedPath..."
        & cmd /c "`"$selectedPath`""
    } else {
        Write-Host "INFO: vcvars64.bat not found in common locations. MSVC might not be fully configured if cl.exe is found via PATH only." -ForegroundColor Yellow
        # Not exiting here, as cl.exe might still be in PATH from a different setup
    }
}

# --- Script Start ---
Write-Host "Starting the build process..." -ForegroundColor Cyan
Write-Host "Build Tool Priority: 1. MSVC (cl.exe), 2. g++, 3. CMake" -ForegroundColor Cyan

# Define project structure and output names for direct compilation
$includeDir = "include"
$srcDir = "src"

$outputMapperDLL = "MapperLib.dll"
$mapperSources = "$srcDir/Mapper_DLL_so.cpp"

$outputReducerDLL = "ReducerLib.dll"
$reducerSources = "$srcDir/Reducer_DLL_so.cpp"

$outputBinary = "MapReduce.exe"
$executableSources = @(
    "$srcDir/main.cpp",
    "$srcDir/ProcessOrchestratorDLL.cpp",
    "$srcDir/Logger.cpp",
    "$srcDir/ErrorHandler.cpp",
    "$srcDir/ThreadPool.cpp",
    "$srcDir/FileHandler.cpp"
)

# Check for available build tools
$useMSVC = Is-CommandAvailable -Command "cl.exe"
$useGpp = $false
$useCMake = $false

if (-not $useMSVC) {
    $useGpp = Is-CommandAvailable -Command "g++"
    if (-not $useGpp) {
        $useCMake = Is-CommandAvailable -Command "cmake"
    }
}

if (-not ($useMSVC -or $useGpp -or $useCMake)) {
    Write-Host "ERROR: No compatible build tools found (MSVC 'cl.exe', 'g++', or 'cmake'). Please install one and ensure it's in your PATH." -ForegroundColor Red
    exit 1
}

# Clean up previous builds (common for direct builds)
if ($useMSVC -or $useGpp) {
    Write-Host "Cleaning up previous direct build artifacts..." -ForegroundColor Yellow
    Clean-File -FilePath $outputMapperDLL
    Clean-File -FilePath "MapperLib.lib" # MSVC import lib
    Clean-File -FilePath "libMapperLib.dll.a" # g++ import lib

    Clean-File -FilePath $outputReducerDLL
    Clean-File -FilePath "ReducerLib.lib" # MSVC import lib
    Clean-File -FilePath "libReducerLib.dll.a" # g++ import lib

    Clean-File -FilePath $outputBinary
    Get-ChildItem -Path . -Include "*.obj", "*.o", "*.exp" -File -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue
} elseif ($useCMake) {
    # For CMake, cleaning is usually handled by 'cmake --build build --target clean' or deleting the build dir
    if (Test-Path "build") {
        Write-Host "Cleaning up previous CMake build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue
    }
}


# --- Build Logic ---

# Build using MSVC (Priority 1)
if ($useMSVC) {
    Write-Host "Attempting to build with MSVC (cl.exe)..." -ForegroundColor Cyan
    Setup-MSVCEnvironment # Attempt to set up full environment

    $msvcCommonCompileFlags = "/EHsc /std:c++17 /MT /nologo /W4"
    $msvcIncludeDirFlag = "/I$includeDir"
    $exportDefineMSVC = "/DDLL_so_EXPORTS"

    # 1. Compile MapperLib.dll
    Write-Host "Compiling $outputMapperDLL..."
    $msvcMapperCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $mapperSources /link /OUT:$outputMapperDLL /IMPLIB:MapperLib.lib"
    if ((Execute-Command -Command $msvcMapperCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 2. Compile ReducerLib.dll
    Write-Host "Compiling $outputReducerDLL..."
    $msvcReducerCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $reducerSources /link /OUT:$outputReducerDLL /IMPLIB:ReducerLib.lib"
    if ((Execute-Command -Command $msvcReducerCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 3. Compile MapReduce.exe
    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $msvcBinaryCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $sourcesString /Fe:$outputBinary MapperLib.lib ReducerLib.lib"
    if ((Execute-Command -Command $msvcBinaryCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 }

    Write-Host "Build completed successfully using MSVC!" -ForegroundColor Green
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "  - Mapper Library: .\$outputMapperDLL (Import: .\MapperLib.lib)" -ForegroundColor Cyan
    Write-Host "  - Reducer Library: .\$outputReducerDLL (Import: .\ReducerLib.lib)" -ForegroundColor Cyan
}
# Build using g++ (Priority 2)
elseif ($useGpp) {
    Write-Host "MSVC not used or failed. Attempting to build with g++..." -ForegroundColor Cyan
    Write-Host "Ensure MinGW g++ bin directory is in your PATH."

    $gppCommonCompileFlags = "-std=c++17 -Wall -Wextra -pthread"
    $gppIncludeDirFlag = "-I$includeDir"
    $exportDefineGPP = "-DDLL_so_EXPORTS"

    # 1. Compile MapperLib.dll
    $outputMapperImportLib_gpp = "libMapperLib.dll.a"
    Write-Host "Compiling $outputMapperDLL (and $outputMapperImportLib_gpp)..."
    $gppMapperCommand = "g++ $gppCommonCompileFlags $gppIncludeDirFlag $exportDefineGPP -shared -fPIC -o $outputMapperDLL $mapperSources -Wl,--out-implib,$outputMapperImportLib_gpp"
    if ((Execute-Command -Command $gppMapperCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 2. Compile ReducerLib.dll
    $outputReducerImportLib_gpp = "libReducerLib.dll.a"
    Write-Host "Compiling $outputReducerDLL (and $outputReducerImportLib_gpp)..."
    $gppReducerCommand = "g++ $gppCommonCompileFlags $gppIncludeDirFlag $exportDefineGPP -shared -fPIC -o $outputReducerDLL $reducerSources -Wl,--out-implib,$outputReducerImportLib_gpp"
    if ((Execute-Command -Command $gppReducerCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 3. Compile MapReduce.exe
    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $gppBinaryCommand = "g++ $gppCommonCompileFlags $gppIncludeDirFlag -o $outputBinary $sourcesString $outputMapperImportLib_gpp $outputReducerImportLib_gpp"
    if ((Execute-Command -Command $gppBinaryCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 }

    Write-Host "Build completed successfully using g++!" -ForegroundColor Green
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "  - Mapper Library: .\$outputMapperDLL (Import: .\$outputMapperImportLib_gpp)" -ForegroundColor Cyan
    Write-Host "  - Reducer Library: .\$outputReducerDLL (Import: .\$outputReducerImportLib_gpp)" -ForegroundColor Cyan
}
# Build using CMake (Priority 3)
elseif ($useCMake) {
    Write-Host "Neither MSVC nor g++ used or failed. Attempting to build with CMake..." -ForegroundColor Cyan
    Write-Host "Ensure you have a valid CMakeLists.txt in the project root." -ForegroundColor Yellow

    if (-not (Test-Path "CMakeLists.txt")) {
        Write-Host "ERROR: CMakeLists.txt not found in the current directory. Cannot proceed with CMake build." -ForegroundColor Red
        exit 1
    }
    
    # Create build directory if it doesn't exist
    if (-not (Test-Path "build")) {
        New-Item -ItemType Directory -Path "build" | Out-Null
    }

    # Generate build system
    Write-Host "Configuring CMake project..."
    $cmakeGenerateCommand = "cmake -S . -B build" # Assuming CMakeLists.txt is in current dir ('.')
    if ((Execute-Command -Command $cmakeGenerateCommand) -ne 0) { Write-Host "ERROR: CMake configuration failed. Exiting." -ForegroundColor Red; exit 1 }

    # Build the project
    Write-Host "Building CMake project..."
    $cmakeBuildCommand = "cmake --build build"
    if ((Execute-Command -Command $cmakeBuildCommand) -ne 0) { Write-Host "ERROR: CMake build failed. Exiting." -ForegroundColor Red; exit 1 }

    Write-Host "Build process completed successfully using CMake!" -ForegroundColor Green
    Write-Host "  - Build artifacts are located in the 'build' directory." -ForegroundColor Cyan
    Write-Host "    (Check CMakeLists.txt for specific output locations of executables/libraries)"
} else {
    # This case should not be reached if the initial tool check is correct.
    Write-Host "ERROR: No suitable build tool was successfully used." -ForegroundColor Red
    exit 1
}
# PowerShell Script to Compile the NewCode Project
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
        Write-Host "ERROR: vcvars64.bat not found. Ensure Visual Studio Build Tools are installed." -ForegroundColor Red
        exit 1
    }
}

# --- Script Start ---
Write-Host "Starting the build process..." -ForegroundColor Cyan
Write-Host "Build Tool Priority: 1. MSVC (cl.exe), 2. g++, 3. CMake" -ForegroundColor Cyan

# Define project structure and output names for direct compilation
$libSearchPaths = @(
    '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.43.34808\lib\x64"',
    '"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64"',
    '"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"'
)

$includeSearchPaths = @(
    '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.43.34808\include"',
    '"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt"',
    '"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um"',
    '"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared"'
)

$srcDir = "src"
$outputMapperDLL = "MapperLib.dll"
$mapperSources = "$srcDir/Mapper_DLL_so.cpp"

$outputReducerDLL = "ReducerLib.dll"
$reducerSources = "$srcDir/Reducer_DLL_so.cpp"

$outputBinary = "MapReduce.exe"
$executableSources = @(
    "$srcDir/main.cpp",
    "$srcDir/ProcessOrchestrator.cpp",
    "$srcDir/ThreadPool.cpp"
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
    $msvcIncludeDirFlag = ($includeSearchPaths | ForEach-Object { "/I$_" }) -join " "
    $msvcLibDirFlag = ($libSearchPaths | ForEach-Object { "/LIBPATH:$_" }) -join " "
    $exportDefineMSVC = "/DDLL_so_EXPORTS"

    # 1. Compile MapperLib.dll
    Write-Host "Compiling $outputMapperDLL..."
    $msvcMapperCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $mapperSources /link /OUT:$outputMapperDLL /IMPLIB:MapperLib.lib $msvcLibDirFlag"
    if ((Execute-Command -Command $msvcMapperCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 2. Compile ReducerLib.dll
    Write-Host "Compiling $outputReducerDLL..."
    $msvcReducerCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $reducerSources /link /OUT:$outputReducerDLL /IMPLIB:ReducerLib.lib $msvcLibDirFlag"
    if ((Execute-Command -Command $msvcReducerCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 3. Compile MapReduce.exe
    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $msvcBinaryCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $sourcesString /Fe:$outputBinary MapperLib.lib ReducerLib.lib $msvcLibDirFlag"
    if ((Execute-Command -Command $msvcBinaryCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 }

    Write-Host "Build completed successfully using MSVC!" -ForegroundColor Green
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "  - Mapper Library: .\$outputMapperDLL (Import: .\MapperLib.lib)" -ForegroundColor Cyan
    Write-Host "  - Reducer Library: .\$outputReducerDLL (Import: .\ReducerLib.lib)" -ForegroundColor Cyan
}
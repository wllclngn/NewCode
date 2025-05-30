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

# Function to execute a shell command in a batch context (for MSVC, so env is set up right)
function Execute-Command-With-MSVCEnv {
    param (
        [string]$MSVCBatchFile,
        [string]$Command
    )
    Write-Host "Executing (with MSVC env): $Command" -ForegroundColor Green
    $fullCommand = "`"$MSVCBatchFile`" && $Command"
    $process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c `"$fullCommand`"" -Wait -NoNewWindow -PassThru
    $exitCode = $process.ExitCode
    if ($exitCode -ne 0) {
        Write-Host "ERROR: Command failed with exit code $exitCode" -ForegroundColor Red
    } else {
        Write-Host "Command executed successfully." -ForegroundColor Green
    }
    return $exitCode
}

# Function to execute a shell command (for non-MSVC tools)
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

# --- Script Start ---
Write-Host "Starting the build process..." -ForegroundColor Cyan
Write-Host "Build Tool Priority: 1. MSVC (cl.exe), 2. g++, 3. CMake" -ForegroundColor Cyan

# Define MSVC batch file (for environment setup)
$vcvarsPathX86 = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$selectedMSVCBatch = ""
if (Test-Path $vcvarsPath) { $selectedMSVCBatch = $vcvarsPath }
elseif (Test-Path $vcvarsPathX86) { $selectedMSVCBatch = $vcvarsPathX86 }

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
    "$srcDir/ProcessOrchestratorDLL.cpp",
    "$srcDir/Logger.cpp",
    "$srcDir/ErrorHandler.cpp",
    "$srcDir/ThreadPool.cpp",
    "$srcDir/FileHandler.cpp"
)

# Check for available build tools
$useMSVC = $false
if ($selectedMSVCBatch) {
    # Use a subshell to test for cl.exe in the MSVC environment
    $testCL = cmd /c "`"$selectedMSVCBatch`" && where cl.exe" 2>$null
    if ($LASTEXITCODE -eq 0 -and $testCL) { $useMSVC = $true }
}
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
    if (-not $selectedMSVCBatch) {
        Write-Host "ERROR: vcvars64.bat not found. Ensure Visual Studio Build Tools are installed." -ForegroundColor Red
        exit 1
    }
    Write-Host "Attempting to build with MSVC (cl.exe)..." -ForegroundColor Cyan

    $msvcCommonCompileFlags = "/EHsc /std:c++17 /MT /nologo /W4"
    $msvcIncludeDirFlag = ($includeSearchPaths | ForEach-Object { "/I$_" }) -join " "
    $msvcLibDirFlag = ($libSearchPaths | ForEach-Object { "/LIBPATH:$_" }) -join " "
    $exportDefineMSVC = "/DDLL_so_EXPORTS"

    # 1. Compile MapperLib.dll
    Write-Host "Compiling $outputMapperDLL..."
    $msvcMapperCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $mapperSources /link /OUT:$outputMapperDLL /IMPLIB:MapperLib.lib $msvcLibDirFlag"
    if ((Execute-Command-With-MSVCEnv -MSVCBatchFile $selectedMSVCBatch -Command $msvcMapperCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 2. Compile ReducerLib.dll
    Write-Host "Compiling $outputReducerDLL..."
    $msvcReducerCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $reducerSources /link /OUT:$outputReducerDLL /IMPLIB:ReducerLib.lib $msvcLibDirFlag"
    if ((Execute-Command-With-MSVCEnv -MSVCBatchFile $selectedMSVCBatch -Command $msvcReducerCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 3. Compile MapReduce.exe
    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $msvcBinaryCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $sourcesString /Fe:$outputBinary MapperLib.lib ReducerLib.lib $msvcLibDirFlag"
    if ((Execute-Command-With-MSVCEnv -MSVCBatchFile $selectedMSVCBatch -Command $msvcBinaryCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 }

    Write-Host "Build completed successfully using MSVC!" -ForegroundColor Green
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "  - Mapper Library: .\$outputMapperDLL (Import: .\MapperLib.lib)" -ForegroundColor Cyan
    Write-Host "  - Reducer Library: .\$outputReducerDLL (Import: .\ReducerLib.lib)" -ForegroundColor Cyan
    exit 0
}

# Build using g++ (Priority 2)
if ($useGpp) {
    Write-Host "Attempting to build with g++..." -ForegroundColor Cyan

    $gppIncludeDirFlag = ($includeSearchPaths | ForEach-Object { "-I$_" }) -join " "
    $gppLibDirFlag = ($libSearchPaths | ForEach-Object { "-L$_" }) -join " "

    # 1. Compile MapperLib.dll
    Write-Host "Compiling $outputMapperDLL..."
    $gppMapperCommand = "g++ -std=c++17 -Wall -Wextra -O2 -shared $gppIncludeDirFlag $mapperSources -o $outputMapperDLL -Wl,--out-implib,libMapperLib.dll.a $gppLibDirFlag"
    if ((Execute-Command -Command $gppMapperCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 2. Compile ReducerLib.dll
    Write-Host "Compiling $outputReducerDLL..."
    $gppReducerCommand = "g++ -std=c++17 -Wall -Wextra -O2 -shared $gppIncludeDirFlag $reducerSources -o $outputReducerDLL -Wl,--out-implib,libReducerLib.dll.a $gppLibDirFlag"
    if ((Execute-Command -Command $gppReducerCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 }

    # 3. Compile MapReduce.exe
    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $gppBinaryCommand = "g++ -std=c++17 -Wall -Wextra -O2 $gppIncludeDirFlag $sourcesString -o $outputBinary -L. -lMapperLib -lReducerLib $gppLibDirFlag"
    if ((Execute-Command -Command $gppBinaryCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 }

    Write-Host "Build completed successfully using g++!" -ForegroundColor Green
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "  - Mapper Library: .\$outputMapperDLL (Import: .\libMapperLib.dll.a)" -ForegroundColor Cyan
    Write-Host "  - Reducer Library: .\$outputReducerDLL (Import: .\libReducerLib.dll.a)" -ForegroundColor Cyan
    exit 0
}

# Build using CMake (Priority 3)
if ($useCMake) {
    Write-Host "Attempting to build with CMake..." -ForegroundColor Cyan
    # Create build directory if missing
    if (-not (Test-Path "build")) { New-Item -ItemType Directory -Path "build" | Out-Null }
    # Configure and build
    $cmakeGen = "cmake -S . -B build"
    $cmakeBuild = "cmake --build build"
    if ((Execute-Command -Command $cmakeGen) -ne 0) { Write-Host "ERROR: CMake configuration failed. Exiting." -ForegroundColor Red; exit 1 }
    if ((Execute-Command -Command $cmakeBuild) -ne 0) { Write-Host "ERROR: CMake build failed. Exiting." -ForegroundColor Red; exit 1 }
    Write-Host "Build completed successfully using CMake!" -ForegroundColor Green
    exit 0
}

Write-Host "ERROR: No build steps completed. This should not happen." -ForegroundColor Red
exit 1
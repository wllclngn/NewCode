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

# Function to prompt the user for custom DLL files
function Get-CustomDLLs {
    $customCleanPaths = @()
    $useCustomDLLs = Read-Host "Do you have custom DLL/LIB files to use (e.g., to replace project's default Mapper/Reducer DLLs)? (yes/no)"
    if ($useCustomDLLs -match '^(y|yes)$') {
        Write-Host "If you provide custom DLLs/LIBs, the script will attempt to use them " -ForegroundColor Yellow
        Write-Host "and skip compiling the project's default MapperLib.dll and ReducerLib.dll." -ForegroundColor Yellow
        Write-Host "For MSVC, if you provide a .dll path, its corresponding .lib will be assumed for linking (e.g., MyLib.dll -> MyLib.lib)." -ForegroundColor Yellow
        while ($true) {
            $userInput = Read-Host "Enter the full path to your DLL or LIB file (or press Enter to finish)"
            if ([string]::IsNullOrWhiteSpace($userInput)) {
                break
            }
            $cleanPath = $userInput.Trim().Trim('"')
            if ([string]::IsNullOrWhiteSpace($cleanPath)) {
                Write-Host "Empty path provided after trimming. Please try again or press Enter to finish." -ForegroundColor Yellow
                continue
            }
            if (Test-Path -Path $cleanPath -PathType Leaf) {
                $customCleanPaths += $cleanPath
            } else {
                Write-Host "Invalid path or not a file: '$cleanPath' (original input: '$userInput'). Please try again." -ForegroundColor Red
            }
        }
    }
    return $customCleanPaths
}

# --- Script Start ---
Write-Host "Starting the build process..." -ForegroundColor Cyan
Write-Host "Build Tool Priority: 1. MSVC (cl.exe), 2. g++, 3. CMake" -ForegroundColor Cyan

$vcvarsPathX86 = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$selectedMSVCBatch = ""
if (Test-Path $vcvarsPath) { $selectedMSVCBatch = $vcvarsPath }
elseif (Test-Path $vcvarsPathX86) { $selectedMSVCBatch = $vcvarsPathX86 }

$customUserCleanPathsArray = Get-CustomDLLs

if ($customUserCleanPathsArray.Count -gt 0) {
    Write-Host "User provided the following library paths (these will be processed for linking):" -ForegroundColor Yellow
    $customUserCleanPathsArray | ForEach-Object { Write-Host "  - $_" }
    Write-Host "Skipping compilation of project's default MapperLib.dll and ReducerLib.dll." -ForegroundColor Yellow
} else {
    Write-Host "No custom libraries provided. Will compile project's default MapperLib.dll and ReducerLib.dll." -ForegroundColor Cyan
}

$projectIncludeDir = "include"
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
$projectMapperLibFileMSVC = "MapperLib.lib"
$projectMapperLibFileGPP = "libMapperLib.dll.a"
$outputReducerDLL = "ReducerLib.dll"
$reducerSources = "$srcDir/Reducer_DLL_so.cpp"
$projectReducerLibFileMSVC = "ReducerLib.lib"
$projectReducerLibFileGPP = "libReducerLib.dll.a"
$outputBinary = "MapReduce.exe"
$executableSources = @(
    "$srcDir/main.cpp",
    "$srcDir/ConfigureManager.cpp",
    "$srcDir/controller.cpp",
    "$srcDir/ProcessOrchestrator.cpp",
    "$srcDir/socket_client.cpp",
    "$srcDir/ThreadPool.cpp",
    "$srcDir/worker_stub.cpp"
)

$useMSVC = $false
if ($selectedMSVCBatch) {
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

# --- Conditional Cleanup ---
Write-Host "Cleaning up previous build artifacts..." -ForegroundColor Yellow
if ($customUserCleanPathsArray.Count -eq 0) {
    Write-Host "  - Cleaning project-specific DLLs/LIBs as they will be recompiled." -ForegroundColor DarkYellow
    Clean-File -FilePath $outputMapperDLL
    Clean-File -FilePath $projectMapperLibFileMSVC
    Clean-File -FilePath $projectMapperLibFileGPP
    Clean-File -FilePath $outputReducerDLL
    Clean-File -FilePath $projectReducerLibFileMSVC
    Clean-File -FilePath $projectReducerLibFileGPP
} else {
    Write-Host "  - Skipping cleanup of project-specific DLLs/LIBs as user is providing them." -ForegroundColor DarkYellow
}
# Always clean the main executable and intermediate object files
Clean-File -FilePath $outputBinary
Get-ChildItem -Path . -Include "*.obj", "*.o", "*.exp" -File -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue

if ($useCMake) {
    if (Test-Path "build") {
        Write-Host "Cleaning up previous CMake build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue
    }
}
# --- End Cleanup ---

if ($useMSVC) {
    if (-not $selectedMSVCBatch) {
        Write-Host "ERROR: vcvars64.bat not found. Ensure Visual Studio Build Tools are installed." -ForegroundColor Red
        exit 1
    }
    Write-Host "Attempting to build with MSVC (cl.exe)..." -ForegroundColor Cyan
    $msvcCommonCompileFlags = "/EHsc /std:c++17 /MT /nologo /W4"
    $msvcIncludeDirFlag = "/I`"$projectIncludeDir`" " + (($includeSearchPaths | ForEach-Object { "/I$_" }) -join " ")
    $msvcLibDirFlag = ($libSearchPaths | ForEach-Object { "/LIBPATH:$_" }) -join " "
    $exportDefineMSVC = "/DDLL_so_EXPORTS"
    $msvcLinkerInputs = @()

    if ($customUserCleanPathsArray.Count -eq 0) {
        Write-Host "Compiling project's $outputMapperDLL..."
        $msvcMapperCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $mapperSources /link /OUT:$outputMapperDLL /IMPLIB:$projectMapperLibFileMSVC $msvcLibDirFlag"
        if ((Execute-Command-With-MSVCEnv -MSVCBatchFile $selectedMSVCBatch -Command $msvcMapperCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 } # Added closing curly brace
        Write-Host "  - Project Mapper Library: .\$outputMapperDLL (Import: .\$projectMapperLibFileMSVC)" -ForegroundColor Cyan
        Write-Host "Compiling project's $outputReducerDLL..."
        $msvcReducerCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $exportDefineMSVC /LD $reducerSources /link /OUT:$outputReducerDLL /IMPLIB:$projectReducerLibFileMSVC $msvcLibDirFlag"
        if ((Execute-Command-With-MSVCEnv -MSVCBatchFile $selectedMSVCBatch -Command $msvcReducerCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 } # Added closing curly brace
        Write-Host "  - Project Reducer Library: .\$outputReducerDLL (Import: .\$projectReducerLibFileMSVC)" -ForegroundColor Cyan
        if ($projectMapperLibFileMSVC.Contains(" ")) { $msvcLinkerInputs += """$projectMapperLibFileMSVC""" } else { $msvcLinkerInputs += $projectMapperLibFileMSVC }
        if ($projectReducerLibFileMSVC.Contains(" ")) { $msvcLinkerInputs += """$projectReducerLibFileMSVC""" } else { $msvcLinkerInputs += $projectReducerLibFileMSVC }
    } else {
        Write-Host "Processing user-provided libraries for MSVC linking:" -ForegroundColor Cyan
        foreach ($cleanPath in $customUserCleanPathsArray) {
            $libPathToLink = $cleanPath
            if ($cleanPath -match '\.dll$') {
                $libPathToLink = $cleanPath -replace '\.dll$', '.lib'
            }
            $quotedLibPath = if ($libPathToLink.Contains(" ")) { """$libPathToLink""" } else { $libPathToLink }
            $msvcLinkerInputs += $quotedLibPath
            Write-Host "  - Will attempt to link with: $quotedLibPath (derived from input: '$cleanPath')"
        }
    }
    $finalLinkLibsMSVCString = $msvcLinkerInputs -join " "

    Write-Host "DEBUG [MSVC Pre-Link Check]: Verifying existence of linker inputs:" -ForegroundColor DarkYellow
    $allMSVCLinkerInputsExist = $true
    foreach ($linkerInput in $msvcLinkerInputs) {
        $pathToCheck = $linkerInput.Trim('"')
        if (-not (Test-Path -Path $pathToCheck -PathType Leaf)) {
            Write-Host "  [ERROR] Linker input NOT FOUND or NOT A FILE: '$pathToCheck' (command line arg: '$linkerInput')" -ForegroundColor Red
            $allMSVCLinkerInputsExist = $false
        } else {
            Write-Host "  [OK] Linker input exists: '$pathToCheck' (command line arg: '$linkerInput')" -ForegroundColor Green
        }
    }
    if (-not $allMSVCLinkerInputsExist) {
        Write-Host "ERROR: One or more linker input files are missing. Please check paths. Aborting MSVC build." -ForegroundColor Red
        exit 1
    }

    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $msvcBinaryCommand = "cl $msvcCommonCompileFlags $msvcIncludeDirFlag $sourcesString /Fe:$outputBinary /link $finalLinkLibsMSVCString $msvcLibDirFlag"
    if ((Execute-Command-With-MSVCEnv -MSVCBatchFile $selectedMSVCBatch -Command $msvcBinaryCommand) -ne 0) { Write-Host "ERROR: MSVC failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 } # Added closing curly brace
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "Build completed successfully using MSVC!" -ForegroundColor Green
    exit 0
} # Added closing curly brace for "if ($useMSVC)"

if ($useGpp) {
    Write-Host "Attempting to build with g++..." -ForegroundColor Cyan
    $gppCommonFlags = "-std=c++17 -Wall -Wextra -O2"
    $gppIncludeDirFlag = "-I`"$projectIncludeDir`" " + (($includeSearchPaths | ForEach-Object { "-I$_" }) -join " ")
    $gppLibDirFlag = ($libSearchPaths | ForEach-Object { "-L$_" }) -join " "
    $exportDefineGPP = "-DDLL_so_EXPORTS"
    $gppLinkerInputs = @()

    if ($customUserCleanPathsArray.Count -eq 0) {
        Write-Host "Compiling project's $outputMapperDLL..."
        $gppMapperCommand = "g++ $gppCommonFlags -shared $gppIncludeDirFlag $exportDefineGPP $mapperSources -o $outputMapperDLL -Wl,--out-implib,$projectMapperLibFileGPP $gppLibDirFlag"
        if ((Execute-Command -Command $gppMapperCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputMapperDLL. Exiting." -ForegroundColor Red; exit 1 }
        Write-Host "  - Project Mapper Library: .\$outputMapperDLL (Import: .\$projectMapperLibFileGPP)" -ForegroundColor Cyan
        Write-Host "Compiling project's $outputReducerDLL..."
        $gppReducerCommand = "g++ $gppCommonFlags -shared $gppIncludeDirFlag $exportDefineGPP $reducerSources -o $outputReducerDLL -Wl,--out-implib,$projectReducerLibFileGPP $gppLibDirFlag"
        if ((Execute-Command -Command $gppReducerCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputReducerDLL. Exiting." -ForegroundColor Red; exit 1 }
        Write-Host "  - Project Reducer Library: .\$outputReducerDLL (Import: .\$projectReducerLibFileGPP)" -ForegroundColor Cyan
        if ($projectMapperLibFileGPP.Contains(" ")) { $gppLinkerInputs += """./$projectMapperLibFileGPP""" } else { $gppLinkerInputs += "./$projectMapperLibFileGPP" }
        if ($projectReducerLibFileGPP.Contains(" ")) { $gppLinkerInputs += """./$projectReducerLibFileGPP""" } else { $gppLinkerInputs += "./$projectReducerLibFileGPP" }
    } else {
        Write-Host "Processing user-provided libraries for g++ linking:" -ForegroundColor Cyan
        foreach ($cleanPath in $customUserCleanPathsArray) {
            $quotedLibPath = if ($cleanPath.Contains(" ")) { """$cleanPath""" } else { $cleanPath }
            $gppLinkerInputs += $quotedLibPath
            Write-Host "  - Will attempt to link with: $quotedLibPath (from input: '$cleanPath')"
        }
    }
    $finalLinkLibsGPPString = $gppLinkerInputs -join " "

    Write-Host "DEBUG [g++ Pre-Link Check]: Verifying existence of linker inputs:" -ForegroundColor DarkYellow
    $allGPPLLinkerInputsExist = $true
    foreach ($linkerInput in $gppLinkerInputs) {
        $pathToCheck = $linkerInput.Trim('"')
        if (-not (Test-Path -Path $pathToCheck -PathType Leaf)) {
            Write-Host "  [ERROR] Linker input NOT FOUND or NOT A FILE: '$pathToCheck' (command line arg: '$linkerInput')" -ForegroundColor Red
            $allGPPLLinkerInputsExist = $false
        } else {
            Write-Host "  [OK] Linker input exists: '$pathToCheck' (command line arg: '$linkerInput')" -ForegroundColor Green
        }
    }
    if (-not $allGPPLLinkerInputsExist) {
        Write-Host "ERROR: One or more linker input files are missing. Please check paths. Aborting g++ build." -ForegroundColor Red
        exit 1
    }

    Write-Host "Compiling $outputBinary..."
    $sourcesString = $executableSources -join ' '
    $gppBinaryCommand = "g++ $gppCommonFlags $gppIncludeDirFlag $sourcesString -o $outputBinary $finalLinkLibsGPPString $gppLibDirFlag"
    if ((Execute-Command -Command $gppBinaryCommand) -ne 0) { Write-Host "ERROR: g++ failed to compile $outputBinary. Exiting." -ForegroundColor Red; exit 1 }
    Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
    Write-Host "Build completed successfully using g++!" -ForegroundColor Green
    exit 0
}

if ($useCMake) {
    Write-Host "Attempting to build with CMake..." -ForegroundColor Cyan
    if (-not (Test-Path "build")) { New-Item -ItemType Directory -Path "build" | Out-Null }
    $cmakeExtraArgs = ""
    if ($customUserCleanPathsArray.Count -gt 0) {
        $cmakeUserLibList = $customUserCleanPathsArray -join ";"
        $cmakeExtraArgs = "-DCUSTOM_USER_LIBRARIES=""$cmakeUserLibList"""
        Write-Host "Passing to CMake: $cmakeExtraArgs" -ForegroundColor Yellow
        Write-Host "Ensure your CMakeLists.txt is configured to use CUSTOM_USER_LIBRARIES variable." -ForegroundColor Yellow
    } else {
         Write-Host "No custom libraries provided to CMake. CMakeLists.txt should build default project libraries." -ForegroundColor Cyan
    }
    $cmakeGen = "cmake -S . -B build $cmakeExtraArgs"
    $cmakeBuild = "cmake --build build"
    if ((Execute-Command -Command $cmakeGen) -ne 0) { Write-Host "ERROR: CMake configuration failed. Exiting." -ForegroundColor Red; exit 1 }
    if ((Execute-Command -Command $cmakeBuild) -ne 0) { Write-Host "ERROR: CMake build failed. Exiting." -ForegroundColor Red; exit 1 }
    Write-Host "Build completed successfully using CMake!" -ForegroundColor Green
    Write-Host "Executable should be in the 'build' directory (e.g., build/Debug/$outputBinary or build/$outputBinary)." -ForegroundColor Cyan
    exit 0
}

Write-Host "ERROR: No build steps completed. This should not happen." -ForegroundColor Red
exit 1
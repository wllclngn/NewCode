# PowerShell Script to Compile the MapReduce Project Using g++ with Pure PowerShell Methods

# Function to check if a command exists (e.g., g++)
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

# Check if g++ is installed
if (-not (Is-CommandAvailable -Command "g++")) {
    Write-Host "ERROR: g++ compiler not found. Please install g++ and try again." -ForegroundColor Red
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

# Compile the shared library (.dll)
Write-Host "Compiling source files into a shared library (.dll)..." -ForegroundColor Cyan
$sharedLibraryCommand = "g++ -std=c++17 -shared -fPIC -o $sharedLibrary $sourceFiles -pthread"
if ((Execute-Command -Command $sharedLibraryCommand) -ne 0) {
    Write-Host "ERROR: Failed to compile the shared library. Exiting." -ForegroundColor Red
    exit 1
}

# Compile the executable binary
Write-Host "Compiling source files into an executable binary..." -ForegroundColor Cyan
$outputBinaryCommand = "g++ -std=c++17 -o $outputBinary $sourceFiles -pthread"
if ((Execute-Command -Command $outputBinaryCommand) -ne 0) {
    Write-Host "ERROR: Failed to compile the executable binary. Exiting." -ForegroundColor Red
    exit 1
}

# Build completed successfully
Write-Host "Build process completed successfully!" -ForegroundColor Green
Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
Write-Host "  - Shared Library: .\$sharedLibrary" -ForegroundColor Cyan
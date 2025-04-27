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


################### OLD #####################

# PowerShell Script Utilizing Embedded C# and g++ Compilation

# EMBEDDED C# CODE TO FIND BOOST LIBRARY
Add-Type -TypeDefinition @"
using System;
using System.IO;

public class BoostFinder
{
    // Method to search for Boost in common directories
    public static string FindBoost()
    {
        string[] commonPaths = {
            @"C:\local\boost_",
            @"C:\Boost",
            @"C:\Program Files\Boost"
        };

        foreach (string basePath in commonPaths)
        {
            if (Directory.Exists(basePath))
            {
                string[] boostDirs = Directory.GetDirectories(basePath, "boost_*");
                if (boostDirs.Length > 0)
                {
                    return boostDirs[0];
                }
            }
        }
        return null;
    }
}
"@

# CALL THE C# METHOD TO FIND THE boost LIBRARY
$boostPath = [BoostFinder]::FindBoost()

if ($null -eq $boostPath) {
    Write-Host "ERROR: boost Library Not Found!" -ForegroundColor Red
    exit 1
}

Write-Host "boost Library Found at: $boostPath" -ForegroundColor Green

# g++ COMMAND FOR COMPILATION
$sourceFile = "mapReduce.cpp"
$outputDLL = "madReduce.dll"
$boostInclude = "$boostPath"

# Construct the g++ command
$gppCommand = @"
g++ -shared -o $outputDLL $sourceFile -I"$boostInclude"
"@

Write-Host "Generated g++ Command: " -ForegroundColor Yellow
Write-Host $gppCommand

# EXECUTE THE G++ COMMAND
# NOTE: Ensure g++ is available in your PATH
Write-Host "Compiling C++ Source File..." -ForegroundColor Cyan
Invoke-Expression $gppCommand

if ($LASTEXITCODE -eq 0) {
    Write-Host "SUCCESS: DLL created, $outputDLL" -ForegroundColor Green
} else {
    Write-Host "ERROR: Check the error messages above." -ForegroundColor Red
}
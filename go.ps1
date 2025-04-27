# PowerShell Script to Compile the MapReduce Project Using g++ with Reusable C# Methods

Add-Type -TypeDefinition @"
using System;
using System.IO;
using System.Diagnostics;

public class BuildHelper
{
    // Method to check if a command exists (e.g., g++)
    public static bool IsCommandAvailable(string command)
    {
        try
        {
            using (Process process = new Process())
            {
                process.StartInfo.FileName = "cmd.exe";
                process.StartInfo.Arguments = $"/c where {command}";
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.Start();
                process.WaitForExit();
                return process.ExitCode == 0;
            }
        }
        catch
        {
            return false;
        }
    }

    // Method to clean up old files
    public static void CleanFile(string filePath)
    {
        if (File.Exists(filePath))
        {
            File.Delete(filePath);
        }
    }

    // Method to execute a shell command
    public static int ExecuteCommand(string command)
    {
        using (Process process = new Process())
        {
            process.StartInfo.FileName = "cmd.exe";
            process.StartInfo.Arguments = $"/c {command}";
            process.StartInfo.RedirectStandardOutput = true;
            process.StartInfo.RedirectStandardError = true;
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.CreateNoWindow = true;

            process.OutputDataReceived += (sender, args) => { if (args.Data != null) Console.WriteLine(args.Data); };
            process.ErrorDataReceived += (sender, args) => { if (args.Data != null) Console.Error.WriteLine(args.Data); };

            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
            process.WaitForExit();
            return process.ExitCode;
        }
    }
}
"@

# Start the build process
Write-Host "Starting the build process..." -ForegroundColor Cyan

# Check if g++ is installed
if (-not [BuildHelper]::IsCommandAvailable("g++")) {
    Write-Host "ERROR: g++ compiler not found. Please install g++ and try again." -ForegroundColor Red
    exit 1
}

# Define source files and output binary
$sourceFiles = "main.cpp mapper.cpp reducer.cpp utils.cpp logger.cpp error_handler.cpp"
$outputBinary = "mapReduce.exe"
$sharedLibrary = "libMapReduce.dll"

# Clean up previous builds
Write-Host "Cleaning up previous builds..." -ForegroundColor Yellow
[BuildHelper]::CleanFile($outputBinary)
[BuildHelper]::CleanFile($sharedLibrary)

# Compile the shared library (.dll)
Write-Host "Compiling source files into a shared library (.dll)..." -ForegroundColor Cyan
$sharedLibraryCommand = "g++ -std=c++17 -shared -fPIC -o $sharedLibrary $sourceFiles -pthread"
Write-Host "Executing: $sharedLibraryCommand" -ForegroundColor Green
if ([BuildHelper]::ExecuteCommand($sharedLibraryCommand) -ne 0) {
    Write-Host "ERROR: Failed to compile the shared library. Exiting." -ForegroundColor Red
    exit 1
}

# Compile the executable binary
Write-Host "Compiling source files into an executable binary..." -ForegroundColor Cyan
$outputBinaryCommand = "g++ -std=c++17 -o $outputBinary $sourceFiles -pthread"
Write-Host "Executing: $outputBinaryCommand" -ForegroundColor Green
if ([BuildHelper]::ExecuteCommand($outputBinaryCommand) -ne 0) {
    Write-Host "ERROR: Failed to compile the executable binary. Exiting." -ForegroundColor Red
    exit 1
}

# Build completed successfully
Write-Host "Build process completed successfully!" -ForegroundColor Green
Write-Host "  - Executable Binary: .\$outputBinary" -ForegroundColor Cyan
Write-Host "  - Shared Library: .\$sharedLibrary" -ForegroundColor Cyan
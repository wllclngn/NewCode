# MapReduce Implementation in C++

## Overview

This project showcases a modern implementation of the **MapReduce** programming model in C++. It is designed for scalable and efficient parallel processing of large datasets. The system can operate in both a single-process interactive mode and a multi-process mode for distributed computation.

---

## Contributors
- **Trevon Carter-Josey**
- **Will Clingan**
- **Junior Kabela**
- **Glen Sherwin**

---

## Features

- **Dual-Mode Operation**:
    - **Interactive Mode**: Single-process execution for smaller datasets or testing.
    - **Multi-Process Mode**: A controller process launches and manages separate mapper and reducer processes for distributed computation.
- **Multi-threaded Processing**: Efficient parallelism within mappers and reducers using a configurable `ThreadPool`. Each pool's thread count can be defined by min/max values or defaults to system hardware concurrency.
- **Dynamic Chunking**: Dynamically calculated chunk sizes for optimal memory usage and load balancing during mapping.
- **Cross-Platform Compatibility**: Designed to work on Windows, Linux, and macOS, with platform-specific build scripts.
- **Custom Logger**: Logs system events with timestamps and mode-specific prefixes.
- **Centralized Error Handling**: A dedicated `ErrorHandler` class for consistent error management.
- **Modular Design**: Code is organized into logical modules for orchestration, mapping (e.g., `Mapper_DLL_so`), reducing (e.g., `Reducer_DLL_so`), file handling, interactive flow, logging, and error management. This design enhances maintainability and is suitable for clear separation of concerns.
- **Build Automation**: Cross-platform build scripts (`go.sh` and `go.ps1`) and CMake support.

---

## Prerequisites

- **C++ Compiler**: A C++17-compliant compiler (e.g., GCC, Clang, MSVC).
- **CMake**: Version 3.10 or later.
- **Build Tools**:
  - Linux/macOS: `make`
  - Windows: Visual Studio or `nmake`
- **Git**: For cloning the repository.

---

## Setup and Installation

### Clone the Repository
```bash
git clone https://github.com/wllclngn/NewCode.git
cd NewCode
```

### Build Instructions

#### Linux/macOS
1. Run the build script:
   ```bash
   ./go.sh
   ```
2. The executable (e.g., `mapreduce`) will be available in the `bin` or a similar build output directory.

#### Windows
1. Open PowerShell and run:
   ```powershell
   ./go.ps1
   ```
2. The executable (e.g., `mapreduce.exe`) will be available in the `bin` or a similar build output directory.

#### Manual Build (Optional)
Use CMake for a manual build:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Usage

The application can be run in several modes: interactive, controller, mapper, or reducer.

### 1. Interactive Mode
If no mode is specified, the application runs interactively, prompting for input, output, and temporary directories.
```bash
./mapreduce
# Follow on-screen prompts
```

### 2. Controller Mode
Orchestrates the MapReduce job by launching mapper and reducer processes.
```bash
./mapreduce controller <inputDir> <outputDir> <tempDir> <M> <R> [<mapperMinPoolThreads> <mapperMaxPoolThreads> <reducerMinPoolThreads> <reducerMaxPoolThreads>] [<successFileName>] [<finalOutputName>] [<partitionPrefix>] [<partitionSuffix>] [controllerLogPath]
```
- `<inputDir>`: Directory containing input text files.
- `<outputDir>`: Directory where final results will be stored.
- `<tempDir>`: Directory for intermediate files.
- `<M>`: Number of mapper processes to launch.
- `<R>`: Number of reducer processes to launch.
- `[<mapperMinPoolThreads>]` (Optional): Minimum threads for mapper thread pools. Defaults to hardware concurrency.
- `[<mapperMaxPoolThreads>]` (Optional): Maximum threads for mapper thread pools. Defaults to hardware concurrency.
- `[<reducerMinPoolThreads>]` (Optional): Minimum threads for reducer thread pools. Defaults to hardware concurrency.
- `[<reducerMaxPoolThreads>]` (Optional): Maximum threads for reducer thread pools. Defaults to hardware concurrency.
- `[<successFileName>]` (Optional): Name of the success indicator file created in the output directory upon job completion. Defaults to `SUCCESS.txt`.
- `[<finalOutputName>]` (Optional): Name of the final aggregated output file. Defaults to `final_results.txt`.
- `[<partitionPrefix>]` (Optional): Prefix for intermediate partition files created by mappers. Defaults to `partition_`.
- `[<partitionSuffix>]` (Optional): Suffix for intermediate partition files (e.g., containing the extension). Defaults to `.txt`.
- `[controllerLogPath]` (Optional): Path for the controller's log file. Defaults to `<outputDir>/controller.log`. (Note: Current `main.cpp` may hardcode a default log path like `MapReduce.log`; this CLI option reflects the intended design for custom controller logging).

**Example (Controller):**
```bash
./mapreduce controller ./input_files ./output_results ./temp_intermediate 4 2 2 8 1 4 job_SUCCESS.txt aggregated_output.txt map_part_ _data.txt ./logs/controller_job.log
```

### 3. Mapper Mode (Typically launched by Controller)
Processes input files and generates intermediate key-value pairs.
```bash
./mapreduce mapper <tempDir> <mapperId> <R> [<minPoolThreads> <maxPoolThreads>] <mapperLogPath> <inputFile1> [inputFile2 ...]
```
- `<tempDir>`: Temporary directory for partitioned intermediate output.
- `<mapperId>`: Unique ID for this mapper instance.
- `<R>`: Total number of reducers (for partitioning).
- `[<minPoolThreads>]` (Optional): Minimum threads for this mapper's thread pool. Defaults to hardware concurrency if not specified by the controller launching it.
- `[<maxPoolThreads>]` (Optional): Maximum threads for this mapper's thread pool. Defaults to hardware concurrency if not specified by the controller launching it.
- `<mapperLogPath>`: Path for this mapper's log file.
- `<inputFile1> ...`: Paths to input files assigned to this mapper.

### 4. Reducer Mode (Typically launched by Controller)
Collects and processes intermediate data for its assigned partition.
```bash
./mapreduce reducer <outputDir> <tempDir> <reducerId> [<minPoolThreads> <maxPoolThreads>] <reducerLogPath>
```
- `<outputDir>`: Directory to write the final output for this partition.
- `<tempDir>`: Temporary directory containing intermediate mapper outputs.
- `<reducerId>`: Unique ID for this reducer instance.
- `[<minPoolThreads>]` (Optional): Minimum threads for this reducer's thread pool. Defaults to hardware concurrency if not specified by the controller launching it.
- `[<maxPoolThreads>]` (Optional): Maximum threads for this reducer's thread pool. Defaults to hardware concurrency if not specified by the controller launching it.
- `<reducerLogPath>`: Path for this reducer's log file.

### Configuration File (`config.txt`)
The repository includes a `config.txt` file which outlines potential settings for thread pools and file naming conventions. However, **this file is not currently used by the application for runtime configuration.** All operational parameters are managed via command-line arguments as described above.

---

## Project Structure (Illustrative)

```
NewCode/
├── src/                     # Or root for .cpp files if not in src
│   ├── main.cpp
│   ├── Mapper_DLL_so.cpp    # If implementation is separate
│   ├── Reducer_DLL_so.cpp   # If implementation is separate
│   └── ...
├── include/                 # Or root for .h files
│   ├── Mapper_DLL_so.h
│   ├── Reducer_DLL_so.h
│   ├── FileHandler.h
│   ├── Logger.h
│   ├── ErrorHandler.h
│   ├── ThreadPool.h
│   ├── ProcessOrchestrator.h # For multi-process logic
│   └── InteractiveMode.h     # For interactive workflow
├── tests/
│   ├── ... (Test files)
├── scripts/
│   ├── go.sh
│   ├── go.ps1
│   └── ... (Test scripts)
├── CHANGELOG.md
├── README.md
└── CMakeLists.txt
```

---

## Testing

Run tests using the provided scripts in the `tests/` or `scripts/` directory. Ensure to test both interactive and controller-driven multi-process modes.

---

## Future Features

- Support for more complex data types.
- Enhanced fault tolerance in multi-process mode.
- Potential integration of `config.txt` for default settings.

---

## License

This project is licensed under the MIT License. See `LICENSE` (if available) for details.

---

## Changelog

Refer to the `CHANGELOG.md` file for updates and changes to the project.
# MapReduce

## Overview
MapReduce is a project designed to demonstrate the MapReduce programming model, which is widely used for processing large datasets. This project includes a multi-threaded implementation of the MapReduce programming paradigm in modern C++.

---

## Features
- **Multi-threaded Mapping and Reducing**: Efficiently processes large datasets using concurrent threads.
- **Cache-Friendly Chunking**: Improves performance by minimizing cache misses.
- **Error Handling and Logging**: Centralized error reporting and thread-safe logging.
- **Modern C++**: Utilizes `std::filesystem` and other C++17 features.
- **Cross-Platform**: Compatible with Linux, MacOS, and Windows.

---

## Prerequisites
- **C++17 or later**
- **CMake 3.10 or later**
- **g++ Compiler (Required for Linux/MacOS builds)**
- **PowerShell (Required for Windows builds)**

---

## Setup Guide

### 1. Clone the Repository
```bash
git clone https://github.com/CSE687-SPRING-2025/MapReduce.git
cd MapReduce
```

### 2. Build the Project

#### Using CMake
```bash
mkdir build && cd build
cmake ..
make
```

#### Using Bash Script (Linux/MacOS)
The Bash script automates the build process. Use it as follows:

```bash
chmod +x go.sh
./go.sh
```

- **What It Does**:
  - Ensures `g++` is installed.
  - Cleans up previous builds.
  - Compiles the project into both an executable binary (`MapReduce`) and a shared library (`libMapReduce.so`).
- **Output**:
  - Executable binary: `./MapReduce`
  - Shared library: `./libMapReduce.so`

#### Using PowerShell Script (Windows)
The PowerShell script automates the build process on Windows. Use it as follows:

```powershell
.\go.ps1
```

- **What It Does**:
  - Checks for the `g++` compiler.
  - Cleans up previous builds.
  - Compiles the project into both an executable binary (`mapReduce.exe`) and a shared library (`libMapReduce.dll`).
- **Output**:
  - Executable binary: `.\mapReduce.exe`
  - Shared library: `.\libMapReduce.dll`

---

## Run the Program
Once the project is built, you can run the program as follows:

### On Linux/MacOS
```bash
./MapReduce
```

### On Windows
```powershell
.\mapReduce.exe
```

---

## Usage
### Input
- Provide a directory containing `.txt` files as input.
- Example:
  ```
  Enter the folder path for the directory to be processed: /path/to/input
  ```

### Output
- Results are written to:
  - `output.txt`: Raw key-value pairs.
  - `output_summed.txt`: Aggregated results.

---

## Contributing
Please see our [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on contributing to this project.

---

## License
This project is licensed under the MIT License.

---

## Contributors
- **Trevon Carter-Josey**
- **Will Clingan**
- **Junior Kabela**
- **Glen Sherwin**
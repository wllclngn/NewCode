# MapReduce Implementation in C++

## Overview

This project showcases a modern implementation of the **MapReduce** programming model in C++. It is designed for scalable and efficient parallel processing of large datasets by splitting the computation into **mapping** and **reducing** phases. The implementation incorporates multi-threading, error handling, and cross-platform compatibility.

---

## Features

- **Multi-threaded Processing**: Efficient parallelism using `std::thread` and `std::mutex` for both mappers and reducers.
- **Dynamic Chunking**: Dynamically calculated chunk sizes for optimal memory usage and load balancing.
- **Cross-Platform Compatibility**: Works seamlessly on Windows, Linux, and macOS, with platform-specific scripts.
- **Custom Logger**: Logs system events with timestamps.
- **Centralized Error Handling**: A dedicated `ErrorHandler` class for consistent error management.
- **Unit Testing**: A built-in testing framework and integration tests for ensuring code quality.
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
git clone https://github.com/<your-org-or-username>/mapreduce-cpp.git
cd mapreduce-cpp
```

### Build Instructions

#### Linux/macOS
1. Run the build script:
   ```bash
   ./go.sh
   ```
2. The executable will be available in the `bin` directory.

#### Windows
1. Open PowerShell and run:
   ```powershell
   ./go.ps1
   ```
2. The executable will be available in the `bin` directory.

#### Manual Build (Optional)
Use CMake for a manual build:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Usage

### Input and Output
- **Input**: A directory containing text files to process.
- **Output**: Word count results generated in an output directory.

### Command-Line Arguments
```bash
mapreduce <input_directory> <output_directory>
```

### Example
```bash
./mapreduce input_files/ output_results/
```

---

## Project Structure

```
mapreduce-cpp/
├── src/
│   ├── Mapper.cpp
│   ├── Reducer.cpp
│   ├── FileHandler.cpp
│   ├── Logger.cpp
│   ├── ErrorHandler.cpp
│   └── main.cpp
├── include/
│   ├── Mapper.h
│   ├── Reducer.h
│   ├── FileHandler.h
│   ├── Logger.h
│   └── ErrorHandler.h
├── tests/
│   ├── TEST_mapper.cpp
│   ├── TEST_reducer.cpp
│   ├── TEST_integration.cpp
│   └── TEST_Test_Framework.h
├── scripts/
│   ├── go.sh
│   ├── go.ps1
│   ├── TEST_BASH_MapReduce.sh
│   └── TEST_PowerShell_MapReduce.ps1
├── CMakeLists.txt
├── README.md
└── CHANGELOG.md
```

---

## Testing

### Running All Tests
To execute all test cases:
```bash
cd tests
./TEST_BASH_MapReduce.sh   # Linux/macOS
./TEST_PowerShell_MapReduce.ps1   # Windows
```

### Adding New Tests
1. Write test cases in the `tests/` directory.
2. Include them in the build process via the `CMakeLists.txt` file.

---

## Advanced Features

### Dynamic Thread Pool (Planned)
Future versions plan to implement a dynamic thread pool for enhanced thread management.

### Distributed Processing (Planned)
Support for distributed processing using MPI or ZeroMQ is under consideration.

---

## Contributing

1. Fork the repository.
2. Create a new branch for your feature or bug fix.
3. Commit your changes with clear and concise messages.
4. Submit a pull request to the `main` branch.

---

## License

This project is licensed under the MIT License. See `LICENSE` for details.

---

## Support

For questions or issues, feel free to open a GitHub issue or contact the maintainer at `<your-email@example.com>`.

---

## Sample Input and Output

### Input Files
The program expects text files in the input directory. For example:
```
input_files/
├── file1.txt
├── file2.txt
└── file3.txt
```

### Output Example
The output files will be stored in the specified output directory. Example file:
```
output_results/
└── word_counts.txt
```

Contents of `word_counts.txt`:
```
word1: 45
word2: 30
word3: 15
```

--- 

## Changelog

See the [CHANGELOG.md](CHANGELOG.md) file for the full history of changes.

---

## Contributors
- **Trevon Carter-Josey**
- **Will Clingan**
- **Junior Kabela**
- **Glen Sherwin**
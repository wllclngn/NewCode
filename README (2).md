# MapReduce

## Overview
MapReduce is a project designed to demonstrate the MapReduce programming model, which is widely used for processing large datasets. This project includes a multi-threaded implementation of the MapReduce pipeline, with features for error handling, logging, and optimized performance.

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

---

## Setup Guide

### 1. Clone the Repository
```bash
git clone https://github.com/CSE687-SPRING-2025/MapReduce.git
cd MapReduce
```

### 2. Build the Project
```bash
mkdir build && cd build
cmake ..
make
```

### 3. Run the Program
```bash
./MapReduce
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
- **William Clingan**
- **Glen Sherwin**
- **Junior Kabela**
- **Trevon Carter-Josey**
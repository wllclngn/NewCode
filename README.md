# MapReduce
A repository for Project #1, MapReduce.

---

## Table of Contents
- [What is MapReduce?](#what-is-mapreduce)
- [How it Works](#how-it-works)
- [Setup Guide](#setup-guide)
- [Usage Guide](#usage-guide)
- [Troubleshooting / FAQ](#troubleshooting--faq)
- [Contributors](#contributors)

---

## What is MapReduce?
MapReduce is a programming model (a way of structuring code to solve a specific type of problem) used for processing and analyzing large amounts of data such as records, files, etc. 

For example, if you have a huge dataset that's too large to fit on a single machine, the MapReduce model solves this issue by:
1. **Mapping**: Breaking the data into smaller chunks or nodes.
2. **Reducing**: Aggregating results from the chunks to produce a final output.

---

## How it Works
1. **Input Files**: The user provides a directory containing text files as input.
2. **Mapping**: Each file is divided into smaller key-value pairs.
3. **Reducing**: Aggregates the key-value pairs (e.g., counting word frequencies).
4. **Output Files**: Results are written to two output files:
   - `output.txt`: Raw word counts.
   - `output_summed.txt`: Summed or aggregated results.

---

## Setup Guide
### Prerequisites
- **C++17 or later** (C++17 for `std::filesystem` if Boost is replaced).
- **Boost Filesystem Library** (if using the current implementation).
- A C++ compiler such as:
  - GCC (Linux)
  - Clang (MacOS)
  - MSVC (Windows)

### Build Instructions
1. Clone the repository:
   ```bash
   git clone https://github.com/CSE687-SPRING-2025/MapReduce.git
   cd MapReduce
   ```
2. Create a build directory and compile the project:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. Run the executable:
   ```bash
   ./MapReduce
   ```

---

## Usage Guide
### Example Workflow
1. **Prepare Input Data**: Create a directory with `.txt` files containing text data.
2. **Run the Program**:
   - You will be prompted to enter:
     - The input directory path.
     - The output directory path.
     - A temporary directory path for intermediate files.
3. **View the Results**:
   - Check the output files in the specified output directory:
     - `output.txt` for key-value pairs.
     - `output_summed.txt` for aggregated results.

### Example Commands
```bash
Enter the folder path for the directory to be processed: /path/to/input
Enter the folder path for the output directory: /path/to/output
Enter the folder path for the temporary directory for intermediate files: /path/to/temp
```

---

## Troubleshooting / FAQ
### 1. "Folder does not exist or is not a directory. Exiting."
- **Solution**: Check that the paths entered are correct and the directories exist.

### 2. "Failed to create fileNames.txt."
- **Solution**: Ensure you have write permissions for the temporary directory.

### 3. "Failed to open `output_summed.txt` for writing."
- **Solution**: Verify that the output directory is writable.

### 4. "Boost library not found."
- **Solution**: Install the Boost library:
  ```bash
  sudo apt-get install libboost-all-dev
  ```

---

## Contributors
- **Trevon Carter-Josey**
- **William Clingan**
- **Junior Kabela**
- **Glen Sherwin**
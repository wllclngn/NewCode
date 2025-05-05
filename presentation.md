# Understanding DLLs in the NewCode Project

## Slide 1: Title and Introduction
- **Title**: "Understanding DLLs in the NewCode Project"
- **Subtitle**: Dynamic Link Libraries and Their Role in Modular Programming
- **Content**:
  - Brief overview of the NewCode project.
  - Key technology: Multi-threaded C++ implementation of MapReduce.

---

## Slide 2: What Are DLLs?
- **Content**:
  - Definition: DLLs are files that contain code and data that can be used by multiple programs simultaneously.
  - Advantages:
    - Code reuse and modularity.
    - Reduced memory footprint.
    - Efficient updates.

---

## Slide 3: DLL Usage in NewCode
- **Content**:
  - The repository includes a class `MapperDLLso` that exports functionality for mapping words.
  - The DLL facilitates cross-platform compatibility, using macros like `__declspec(dllexport)` (Windows) and `__attribute__((visibility("default")))` (Linux).

---

## Slide 4: Key Functions in `MapperDLLso`
- **Content**:
  - `is_valid_char(char c)`: Validates characters for word processing.
  - `clean_word(const std::string &word)`: Cleans input strings to extract meaningful words.
  - `map_words(const std::vector<std::string> &lines, const std::string &tempFolderPath)`: Processes text lines and maps words to their frequencies.

---

## Slide 5: Cross-Platform Build Process
- **Content**:
  - The repository contains PowerShell (`go.ps1`) and Bash (`go.sh`) scripts for building DLLs.
  - Highlights:
    - Use of MSVC (`cl.exe`) on Windows.
    - Use of `g++` and `CMake` on Linux/macOS.

---

## Slide 6: Example Workflow
- **Content**:
  - Input: Text lines provided to the `map_words` function.
  - Processing: DLL logic validates and cleans words, then maps them to frequency.
  - Output: Results are written to a temporary file (`mapped_temp.txt`).

---

## Slide 7: Testing and Validation
- **Content**:
  - Example test file: `TEST_Mapper_DLL_so.cpp`.
  - Verifies:
    - Correct mapping logic using the DLL.
    - Cross-platform functionality.

---

## Slide 8: Benefits of Using DLLs in NewCode
- **Content**:
  - Modular architecture allows for scalable updates and maintenance.
  - Improved code sharing across different components.
  - Simplifies building and deploying the software on multiple operating systems.

---

## Slide 9: Challenges and Considerations
- **Content**:
  - Managing dependencies across platforms.
  - Ensuring compatibility with different compilers and linkers.
  - Debugging and testing DLL-specific issues.

---

## Slide 10: Summary and Q&A
- **Content**:
  - Recap the role of DLLs in NewCode.
  - Highlight their contribution to modularity and cross-platform compatibility.
  - Open the floor for questions.
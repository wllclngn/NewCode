# Plan to Address Compilation Errors (with Lines/Files)

Below is a point-by-point plan for addressing each **error** from your compilation output, now including exact lines or requests for corrected files as needed.  
**For any step, if you need the exact code or the full corrected file, just ask for that item specifically.**

---

## 1. `'ends_with': is not a member of 'std::basic_string'`
- **File:** `include/InteractiveMode.h`  
- **Line:** 57  
- **Action:**  
  - Find all usages of `.ends_with(...)` on a `std::string` in `include/InteractiveMode.h`.
  - Replace with a top-level (outside any class) helper function:
    ```cpp
    inline bool ends_with(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    ```
  - Change all `mystring.ends_with(suffix)` to `ends_with(mystring, suffix)`.
  - **If you want a fully corrected `InteractiveMode.h`, just ask.**

---

## 2. `'MapperDLLso': undeclared identifier` and related errors
- **File:** `include/InteractiveMode.h`  
- **Line:** 107 (and likely 108)  
- **Action:**  
  - Make sure the class `MapperDLLso` is declared and included (e.g., `#include "Mapper_DLL_so.h"`).
  - If the class is actually named `Mapper`, update the declaration and usage to use `Mapper`.
  - For example, change:
    ```cpp
    MapperDLLso mapper;
    ```
    to
    ```cpp
    Mapper mapper;
    ```
  - **If you want corrected lines or the full corrected file, just ask.**

---

## 3. `'ReducerDLLso::~ReducerDLLso': cannot access private member declared in class 'ReducerDLLso'`
- **File:** `include/Reducer_DLL_so.h`
- **Line:** 19 (declaration), referenced in `InteractiveMode.h` line 124  
- **Action:**  
  - Open `include/Reducer_DLL_so.h`.
  - Make sure the destructor `~ReducerDLLso();` is declared in the `public:` section.
  - Move it if it is under `private:` or `protected:`.
  - **If you want a corrected `Reducer_DLL_so.h`, just ask.**

---

## 4. `'ReducerDLLso::reduce': cannot access private member declared in class 'ReducerDLLso'`
- **File:** `include/Reducer_DLL_so.h`
- **Line:** 21 (declaration), referenced in `InteractiveMode.h` line 125  
- **Action:**  
  - In `include/Reducer_DLL_so.h`, ensure `void reduce(...);` is in the `public:` section of the class.
  - Move it if it is under `private:` or `protected:`.
  - **If you want a corrected `Reducer_DLL_so.h`, just ask.**

---

## 5. `'parseMode': identifier not found`
- **File:** `src/main.cpp`  
- **Line:** 63  
- **Action:**  
  - At the top of `src/main.cpp` (before `main`), add a function:
    ```cpp
    AppMode parseMode(const std::string& modeStr) {
        if (modeStr == "controller") return AppMode::CONTROLLER;
        if (modeStr == "mapper") return AppMode::MAPPER;
        if (modeStr == "reducer") return AppMode::REDUCER;
        if (modeStr == "final_reducer") return AppMode::FINAL_REDUCER;
        if (modeStr == "interactive") return AppMode::INTERACTIVE;
        return AppMode::UNKNOWN;
    }
    ```
  - Or, include the header that declares this function.
  - **If you want an updated `main.cpp` with this function added, just ask.**

---

## 6. `'ProcessOrchestratorDLL::runFinalReducer': cannot access private member declared in class 'ProcessOrchestratorDLL'`
- **File:** `include/ProcessOrchestrator.h`
- **Line:** 79 (declaration), referenced in `main.cpp` line 123  
- **Action:**  
  - In `include/ProcessOrchestrator.h`, move the declaration of `void runFinalReducer(...);` to the `public:` section of the `ProcessOrchestratorDLL` class.
  - **If you want a corrected `ProcessOrchestrator.h`, just ask.**

---

## 7. `std::lock_guard<std::mutex>` errors (cannot convert argument 1 from 'const std::mutex' to '_Mutex &')
- **File:** `src/ThreadPool.cpp`
- **Lines:** 77, 83  
- **Action:**  
  - For each error line, ensure you are passing a non-const mutex to `std::lock_guard`:
    ```cpp
    std::lock_guard<std::mutex> lock(queueMutex); // Not const
    ```
  - Make sure your mutex member variables are not declared as `const` in the class definition.
  - **If you want a corrected `ThreadPool.cpp`, just ask.**

---

**For each step, if you need the exact lines, a function implementation, or a corrected file, just request that file or section!**
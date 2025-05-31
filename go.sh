#!/bin/bash

# --- Configuration ---
SCRIPT_VERSION="1.0.0"
COLOR_CYAN='\033[0;36m'
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_YELLOW='\033[0;33m'
COLOR_DARK_YELLOW='\033[1;33m' # Technically bright yellow, using for emphasis
COLOR_NC='\033[0m' # No Color

# Project structure and source files
PROJECT_INCLUDE_DIR="include"
SRC_DIR="src"

MAPPER_SOURCES="$SRC_DIR/Mapper_DLL_so.cpp"
REDUCER_SOURCES="$SRC_DIR/Reducer_DLL_so.cpp" # Corrected typo from Reducerr
# Ensure these additional source files exist in your src/ directory
EXECUTABLE_SOURCES=(
    "$SRC_DIR/main.cpp"
    "$SRC_DIR/ConfigureManager.cpp"
    "$SRC_DIR/ProcessOrchestrator.cpp"
    "$SRC_DIR/ThreadPool.cpp"
)

OUTPUT_BINARY_NAME="MapReduce" # Changed from mapreduce_controller to match .exe

# Platform-dependent names
if [[ "$(uname)" == "Darwin" ]]; then
    OUTPUT_MAPPER_LIB_NAME="MapperLib.dylib"
    OUTPUT_REDUCER_LIB_NAME="ReducerLib.dylib"
else
    OUTPUT_MAPPER_LIB_NAME="MapperLib.so"
    OUTPUT_REDUCER_LIB_NAME="ReducerLib.so"
fi

# Compiler and Linker flags
# -fPIC is necessary for shared libraries.
# -Wall -Wextra -O2 are good general flags.
# -pthread for thread support
# Define DLL_so_EXPORTS when building the shared libraries themselves
CXX_COMMON_FLAGS="-std=c++17 -Wall -Wextra -O2 -pthread"
CXX_SHARED_LIB_FLAGS="$CXX_COMMON_FLAGS -fPIC -DDLL_so_EXPORTS"
CXX_EXECUTABLE_FLAGS="$CXX_COMMON_FLAGS"
LD_FLAGS="-pthread"
INCLUDE_PATHS="-I$PROJECT_INCLUDE_DIR -I$SRC_DIR" # Assuming headers might also be in src or root

# --- Helper Functions ---
print_message() {
    local color="$1"
    local message="$2"
    echo -e "${color}${message}${COLOR_NC}"
}

is_command_available() {
    command -v "$1" >/dev/null 2>&1
}

execute_command() {
    print_message "$COLOR_GREEN" "Executing command: $*"
    "$@"
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        print_message "$COLOR_RED" "ERROR: Command failed with exit code $exit_code: $*"
    else
        print_message "$COLOR_GREEN" "Command executed successfully."
    fi
    return $exit_code
}

clean_file() {
    if [ -f "$1" ]; then
        print_message "$COLOR_DARK_YELLOW" "  - Cleaning file: $1"
        rm -f "$1"
    fi
}

get_custom_libs() {
    CUSTOM_MAPPER_LIB_PATH=""
    CUSTOM_REDUCER_LIB_PATH=""
    USE_CUSTOM_LIBS_ANSWER=""

    print_message "$COLOR_YELLOW" "This script can compile the project's Mapper and Reducer libraries, or you can provide paths to your own pre-compiled versions."
    read -r -p "Do you want to use custom pre-compiled Mapper and Reducer shared libraries? (yes/no): " USE_CUSTOM_LIBS_ANSWER

    if [[ "$USE_CUSTOM_LIBS_ANSWER" =~ ^[Yy]([Ee][Ss])?$ ]]; then
        print_message "$COLOR_YELLOW" "You chose to use custom libraries."
        print_message "$COLOR_YELLOW" "The script will skip compiling the project's default $OUTPUT_MAPPER_LIB_NAME and $OUTPUT_REDUCER_LIB_NAME."
        
        while true; do
            read -r -p "Enter the full path to your custom Mapper shared library (e.g., /path/to/MyMapper.so or .dylib): " INPUT_MAPPER_LIB
            if [ -z "$INPUT_MAPPER_LIB" ]; then
                print_message "$COLOR_RED" "Mapper library path cannot be empty if you choose to provide one. Defaulting to project build."
                USE_CUSTOM_LIBS_ANSWER="no" # Revert if one is empty
                break
            elif [ -f "$INPUT_MAPPER_LIB" ]; then
                CUSTOM_MAPPER_LIB_PATH="$INPUT_MAPPER_LIB"
                print_message "$COLOR_GREEN" "  - Using custom Mapper library: $CUSTOM_MAPPER_LIB_PATH"
                break
            else
                print_message "$COLOR_RED" "Invalid path or not a file: '$INPUT_MAPPER_LIB'. Please try again."
            fi
        done

        if [[ "$USE_CUSTOM_LIBS_ANSWER" =~ ^[Yy]([Ee][Ss])?$ ]]; then # Check again in case it was reverted
            while true; do
                read -r -p "Enter the full path to your custom Reducer shared library (e.g., /path/to/MyReducer.so or .dylib): " INPUT_REDUCER_LIB
                if [ -z "$INPUT_REDUCER_LIB" ]; then
                    print_message "$COLOR_RED" "Reducer library path cannot be empty if you choose to provide one. Defaulting to project build."
                    USE_CUSTOM_LIBS_ANSWER="no" # Revert
                    CUSTOM_MAPPER_LIB_PATH="" # Clear mapper too if reducer is not provided
                    break
                elif [ -f "$INPUT_REDUCER_LIB" ]; then
                    CUSTOM_REDUCER_LIB_PATH="$INPUT_REDUCER_LIB"
                    print_message "$COLOR_GREEN" "  - Using custom Reducer library: $CUSTOM_REDUCER_LIB_PATH"
                    break
                else
                    print_message "$COLOR_RED" "Invalid path or not a file: '$INPUT_REDUCER_LIB'. Please try again."
                fi
            done
        fi
    fi
    if [[ ! "$USE_CUSTOM_LIBS_ANSWER" =~ ^[Yy]([Ee][Ss])?$ ]]; then
        print_message "$COLOR_CYAN" "No valid custom libraries provided or selected. Will compile project's default libraries."
        CUSTOM_MAPPER_LIB_PATH=""
        CUSTOM_REDUCER_LIB_PATH=""
        return 1 # Indicates project libraries should be built
    fi
    return 0 # Indicates custom libraries will be used
}


# --- Script Start ---
print_message "$COLOR_CYAN" "Starting the build process for NewCode (Unix)..."
print_message "$COLOR_CYAN" "Build Tool Priority: 1. g++, 2. CMake"

# Tool detection
USE_GPP=false
USE_CMAKE=false

if is_command_available "g++"; then
    USE_GPP=true
    print_message "$COLOR_GREEN" "g++ found."
elif is_command_available "cmake"; then
    USE_CMAKE=true
    print_message "$COLOR_GREEN" "cmake found."
else
    print_message "$COLOR_RED" "ERROR: No compatible build tools found (g++ or cmake). Please install one and ensure it's in your PATH."
    exit 1
fi

# Get custom library preferences
get_custom_libs
CUSTOM_LIBS_PROVIDED=$? # 0 if custom libs are used, 1 if project libs are to be built

# --- Conditional Cleanup ---
print_message "$COLOR_YELLOW" "Cleaning up previous build artifacts..."
if [ $CUSTOM_LIBS_PROVIDED -ne 0 ]; then # Building project's libs
    print_message "$COLOR_DARK_YELLOW" "  - Cleaning project-specific shared libraries as they will be recompiled."
    clean_file "$OUTPUT_MAPPER_LIB_NAME"
    clean_file "$OUTPUT_REDUCER_LIB_NAME"
else
    print_message "$COLOR_DARK_YELLOW" "  - Skipping cleanup of project-specific shared libraries as user is providing them."
fi
# Always clean the main executable and object files
clean_file "$OUTPUT_BINARY_NAME"
find . -maxdepth 1 -name "*.o" -exec rm -f {} \; # Clean .o files in current dir

if $USE_CMAKE; then
    if [ -d "build" ]; then
        print_message "$COLOR_YELLOW" "Cleaning up previous CMake build directory..."
        rm -rf "build"
    fi
fi
# --- End Cleanup ---

LINKER_INPUTS=() # Array to hold paths to .so/.dylib files for linking

if $USE_GPP; then
    print_message "$COLOR_CYAN" "Attempting to build with g++..."

    if [ $CUSTOM_LIBS_PROVIDED -ne 0 ]; then # Build project's libraries
        print_message "$COLOR_CYAN" "Compiling project's $OUTPUT_MAPPER_LIB_NAME..."
        CMD_MAPPER="g++ $CXX_SHARED_LIB_FLAGS -shared -o $OUTPUT_MAPPER_LIB_NAME $MAPPER_SOURCES $INCLUDE_PATHS $LD_FLAGS"
        execute_command $CMD_MAPPER || { print_message "$COLOR_RED" "ERROR: g++ failed to compile $OUTPUT_MAPPER_LIB_NAME. Exiting."; exit 1; }
        if [[ "$(uname)" == "Darwin" ]]; then
            execute_command install_name_tool -id "@rpath/$OUTPUT_MAPPER_LIB_NAME" "$OUTPUT_MAPPER_LIB_NAME"
        fi
        LINKER_INPUTS+=("./$OUTPUT_MAPPER_LIB_NAME")

        print_message "$COLOR_CYAN" "Compiling project's $OUTPUT_REDUCER_LIB_NAME..."
        CMD_REDUCER="g++ $CXX_SHARED_LIB_FLAGS -shared -o $OUTPUT_REDUCER_LIB_NAME $REDUCER_SOURCES $INCLUDE_PATHS $LD_FLAGS"
        execute_command $CMD_REDUCER || { print_message "$COLOR_RED" "ERROR: g++ failed to compile $OUTPUT_REDUCER_LIB_NAME. Exiting."; exit 1; }
        if [[ "$(uname)" == "Darwin" ]]; then
            execute_command install_name_tool -id "@rpath/$OUTPUT_REDUCER_LIB_NAME" "$OUTPUT_REDUCER_LIB_NAME"
        fi
        LINKER_INPUTS+=("./$OUTPUT_REDUCER_LIB_NAME")
    else # Use custom libraries
        print_message "$COLOR_CYAN" "Using user-provided custom libraries for linking:"
        print_message "$COLOR_GREEN" "  - Mapper: $CUSTOM_MAPPER_LIB_PATH"
        print_message "$COLOR_GREEN" "  - Reducer: $CUSTOM_REDUCER_LIB_PATH"
        LINKER_INPUTS+=("$CUSTOM_MAPPER_LIB_PATH")
        LINKER_INPUTS+=("$CUSTOM_REDUCER_LIB_PATH")
    fi

    print_message "$COLOR_DARK_YELLOW" "DEBUG [g++ Pre-Link Check]: Verifying existence of linker inputs:"
    ALL_LINKER_INPUTS_EXIST=true
    for linker_input in "${LINKER_INPUTS[@]}"; do
        if [ ! -f "$linker_input" ]; then
            print_message "$COLOR_RED" "  [ERROR] Linker input NOT FOUND or NOT A FILE: '$linker_input'"
            ALL_LINKER_INPUTS_EXIST=false
        else
            print_message "$COLOR_GREEN" "  [OK] Linker input exists: '$linker_input'"
        fi
    done
    if ! $ALL_LINKER_INPUTS_EXIST; then
        print_message "$COLOR_RED" "ERROR: One or more linker input files are missing. Please check paths. Aborting g++ build."
        exit 1
    fi
    
    RPath_Setting=""
    if [[ "$(uname)" == "Darwin" ]]; then
        RPath_Setting="-Wl,-rpath,@executable_path"
    else # Linux
        RPath_Setting="-Wl,-rpath,'\$ORIGIN'"
    fi

    print_message "$COLOR_CYAN" "Compiling $OUTPUT_BINARY_NAME..."
    # Convert array of executable sources to string
    SOURCES_STRING="${EXECUTABLE_SOURCES[*]}"
    CMD_BINARY="g++ $CXX_EXECUTABLE_FLAGS -o $OUTPUT_BINARY_NAME $SOURCES_STRING $INCLUDE_PATHS ${LINKER_INPUTS[*]} $LD_FLAGS $RPath_Setting"
    execute_command $CMD_BINARY || { print_message "$COLOR_RED" "ERROR: g++ failed to compile $OUTPUT_BINARY_NAME. Exiting."; exit 1; }
    
    print_message "$COLOR_GREEN" "  - Executable Binary: ./$OUTPUT_BINARY_NAME"
    print_message "$COLOR_GREEN" "Build completed successfully using g++!"
    exit 0

elif $USE_CMAKE; then
    print_message "$COLOR_CYAN" "Attempting to build with CMake..."
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    CMAKE_EXTRA_ARGS=""
    if [ $CUSTOM_LIBS_PROVIDED -eq 0 ]; then # Custom libs are provided
        print_message "$COLOR_YELLOW" "Passing custom library paths to CMake:"
        print_message "$COLOR_YELLOW" "  - CUSTOM_MAPPER_LIB_PATH=$CUSTOM_MAPPER_LIB_PATH"
        print_message "$COLOR_YELLOW" "  - CUSTOM_REDUCER_LIB_PATH=$CUSTOM_REDUCER_LIB_PATH"
        CMAKE_EXTRA_ARGS="-DCUSTOM_MAPPER_LIB_PATH=\"$CUSTOM_MAPPER_LIB_PATH\" -DCUSTOM_REDUCER_LIB_PATH=\"$CUSTOM_REDUCER_LIB_PATH\" -DUSE_CUSTOM_PROJECT_LIBS=ON"
        print_message "$COLOR_YELLOW" "Ensure your CMakeLists.txt is configured to use these variables when USE_CUSTOM_PROJECT_LIBS is ON."
    else
        CMAKE_EXTRA_ARGS="-DUSE_CUSTOM_PROJECT_LIBS=OFF"
        print_message "$COLOR_CYAN" "No custom libraries provided to CMake. CMakeLists.txt should build default project libraries."
    fi

    CMD_CMAKE_GEN="cmake -S . -B build $CMAKE_EXTRA_ARGS"
    CMD_CMAKE_BUILD="cmake --build build"

    execute_command $CMD_CMAKE_GEN || { print_message "$COLOR_RED" "ERROR: CMake configuration failed. Exiting."; exit 1; }
    execute_command $CMD_CMAKE_BUILD || { print_message "$COLOR_RED" "ERROR: CMake build failed. Exiting."; exit 1; }
    
    print_message "$COLOR_GREEN" "Build completed successfully using CMake!"
    print_message "$COLOR_CYAN" "Executable should be in the 'build' directory (e.g., build/$OUTPUT_BINARY_NAME or build/Debug/$OUTPUT_BINARY_NAME)."
    print_message "$COLOR_CYAN" "Look for $OUTPUT_BINARY_NAME within the build directory structure."
    exit 0
fi

print_message "$COLOR_RED" "ERROR: No build steps completed. This should not happen."
exit 1
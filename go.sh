#!/bin/bash

# Define source files and output targets
# These source files are expected to be in the 'src/' directory.
# Add other .cpp files to DLL_SOURCE_FILES if they are part of the DLL's core logic
# and not just header-only utilities included by Mapper.cpp or Reducer.cpp.
DLL_SOURCE_FILES="src/Mapper_DLL_so.cpp src/Reducerr_DLL_so.cpp"
EXECUTABLE_SOURCE_FILES="src/main.cpp"

OUTPUT_CONTROLLER_BINARY="mapreduce_controller"

# Detect platform and set shared library extension
if [[ "$(uname)" == "Darwin" ]]; then
    SHARED_LIBRARY_NAME="LibMapReduce.dylib"
else
    SHARED_LIBRARY_NAME="LibMapReduce.so"
fi

# Include paths - adjust if your headers are elsewhere.
# Assumes headers like Mapper_DLL_so.h are either in the root, src/, or include/
INCLUDE_PATHS="-I. -Iinclude -Isrc"

# Compiler and Linker flags
# -fPIC is necessary for shared libraries.
# -Wall -Wextra -O2 are good general flags for development/release.
CXX_FLAGS="-std=c++17 -pthread -fPIC -Wall -Wextra -O2"
LD_FLAGS="-pthread" # General linker flags

# Prompt the user for custom EXTERNAL shared libraries to link against
read -p "Do you have custom EXTERNAL shared libraries you'd like to link (e.g., path to another .so/.dylib)? (yes/no): " INCLUDE_CUSTOM_LIBS
CUSTOM_LINK_LIBS_ARGS="" # Arguments for g++ for custom libraries

if [[ "$INCLUDE_CUSTOM_LIBS" == "yes" ]]; then
    echo "Enter the full path to your EXTERNAL shared libraries, one per line."
    echo "Press Enter on an empty line when you're done:"
    while true; do
        read -p "Path to shared library (e.g., /path/to/your_external_lib.so): " LIB_PATH_INPUT
        if [[ -z "$LIB_PATH_INPUT" ]]; then
            break
        fi
        if [[ -f "$LIB_PATH_INPUT" ]]; then
            CUSTOM_LINK_LIBS_ARGS+=" $LIB_PATH_INPUT" # Add direct path to the library
        else
            echo "Invalid path: $LIB_PATH_INPUT. The file does not exist. Please try again."
        fi
    done
fi

# Clean previous builds
echo "Cleaning previous builds..."
rm -f "$SHARED_LIBRARY_NAME" "$OUTPUT_CONTROLLER_BINARY"
# Optionally, clean specific object files if any: rm -f src/*.o

# Compile shared library (LibMapReduce)
echo "Compiling shared library $SHARED_LIBRARY_NAME from $DLL_SOURCE_FILES..."
g++ $CXX_FLAGS -shared -o "$SHARED_LIBRARY_NAME" $DLL_SOURCE_FILES $INCLUDE_PATHS
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile shared library $SHARED_LIBRARY_NAME."
    exit 1
fi

# macOS-specific: Set install_name for the shared library
# This helps the dynamic linker find the library if it's in a non-standard location,
# often used with @rpath.
if [[ "$(uname)" == "Darwin" ]]; then
    echo "Setting install_name for $SHARED_LIBRARY_NAME..."
    install_name_tool -id @rpath/$SHARED_LIBRARY_NAME "$SHARED_LIBRARY_NAME"
fi

# Compile executable binary (Controller)
echo "Compiling executable binary $OUTPUT_CONTROLLER_BINARY from $EXECUTABLE_SOURCE_FILES..."
# Link the executable against the shared library created in the previous step.
# "./$SHARED_LIBRARY_NAME" provides a direct path to the library.
# -Wl,-rpath,'$ORIGIN' tells the linker to add the executable's directory to the runtime library search path.
# This allows the executable to find the shared library if it's in the same directory.
g++ $CXX_FLAGS -o "$OUTPUT_CONTROLLER_BINARY" $EXECUTABLE_SOURCE_FILES $INCLUDE_PATHS "./$SHARED_LIBRARY_NAME" $LD_FLAGS $CUSTOM_LINK_LIBS_ARGS -Wl,-rpath,'$ORIGIN'

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile executable binary $OUTPUT_CONTROLLER_BINARY."
    exit 1
fi

echo "Build process completed successfully."
echo "  - Shared Library: ./$SHARED_LIBRARY_NAME"
echo "  - Controller Executable: ./$OUTPUT_CONTROLLER_BINARY"
echo "You can run the controller with: ./$OUTPUT_CONTROLLER_BINARY <input_dir> <output_dir> <temp_dir> <M> <R>"

if [[ -n "$CUSTOM_LINK_LIBS_ARGS" ]]; then
    echo "Custom external shared libraries linked:"
    echo "$CUSTOM_LINK_LIBS_ARGS"
fi
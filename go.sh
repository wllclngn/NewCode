#!/bin/bash

# Define source files and output targets
SOURCE_FILES="main.cpp"
OUTPUT_BINARY="MapReduce"

# Detect platform and set shared library extension
if [[ "$(uname)" == "Darwin" ]]; then
    SHARED_LIBRARY="LibMapReduce.dylib"
else
    SHARED_LIBRARY="LibMapReduce.so"
fi

# Clean previous builds
echo "Cleaning previous builds..."
rm -f "$OUTPUT_BINARY" "$SHARED_LIBRARY"

# Compile shared library
echo "Compiling shared library $SHARED_LIBRARY..."
g++ -std=c++17 -shared -fPIC -o "$SHARED_LIBRARY" $SOURCE_FILES -pthread
if [ $? -ne 0 ]; then
    echo "Failed to compile shared library $SHARED_LIBRARY."
    exit 1
fi

# macOS-specific: Set install_name for the shared library
if [[ "$(uname)" == "Darwin" ]]; then
    echo "Setting install_name for $SHARED_LIBRARY..."
    install_name_tool -id @rpath/$SHARED_LIBRARY $SHARED_LIBRARY
fi

# Compile executable binary
echo "Compiling executable binary $OUTPUT_BINARY..."
g++ -std=c++17 -o "$OUTPUT_BINARY" $SOURCE_FILES -pthread
if [ $? -ne 0 ]; then
    echo "Failed to compile executable binary $OUTPUT_BINARY."
    exit 1
fi

echo "Build process completed successfully."
echo "You can run the program with ./$OUTPUT_BINARY or use the shared library $SHARED_LIBRARY."
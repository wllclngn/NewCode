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

# Prompt the user for custom shared libraries
read -p "Do you have custom shared libraries you'd like to include? (yes/no): " INCLUDE_CUSTOM_LIBS
CUSTOM_LIBS=""
if [[ "$INCLUDE_CUSTOM_LIBS" == "yes" ]]; then
    echo "Enter the full path to your shared libraries, one per line."
    echo "Press Enter on an empty line when you're done:"
    while true; do
        read -p "Path to shared library: " LIB_PATH
        if [[ -z "$LIB_PATH" ]]; then
            break
        fi
        if [[ -f "$LIB_PATH" ]]; then
            CUSTOM_LIBS+=" $LIB_PATH"
        else
            echo "Invalid path. The file does not exist. Please try again."
        fi
    done
fi

# Clean previous builds
echo "Cleaning previous builds..."
rm -f "$OUTPUT_BINARY" "$SHARED_LIBRARY"

# Compile shared library
echo "Compiling shared library $SHARED_LIBRARY..."
g++ -std=c++17 -shared -fPIC -o "$SHARED_LIBRARY" $SOURCE_FILES -pthread $CUSTOM_LIBS
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
g++ -std=c++17 -o "$OUTPUT_BINARY" $SOURCE_FILES -pthread $CUSTOM_LIBS
if [ $? -ne 0 ]; then
    echo "Failed to compile executable binary $OUTPUT_BINARY."
    exit 1
fi

echo "Build process completed successfully."
echo "You can run the program with ./$OUTPUT_BINARY or use the shared library $SHARED_LIBRARY."
if [[ -n "$CUSTOM_LIBS" ]]; then
    echo "Custom shared libraries included:"
    echo "$CUSTOM_LIBS"
fi
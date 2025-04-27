#!/bin/bash

# Script to compile the MapReduce project using g++

echo "Starting the build process..."

# Ensure g++ is installed
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ is not installed. Please install it and try again."
    exit 1
fi

# Define source files and output binary
SOURCE_FILES="main.cpp mapper.cpp reducer.cpp fileHandler.cpp utils.cpp"
OUTPUT_BINARY="MapReduce"

# Clean previous builds
if [ -f "$OUTPUT_BINARY" ]; then
    echo "Cleaning previous build..."
    rm -f "$OUTPUT_BINARY"
fi

# Compile the project
echo "Compiling source files..."
g++ -std=c++17 -o "$OUTPUT_BINARY" $SOURCE_FILES -pthread

# Check for success
if [ $? -eq 0 ]; then
    echo "Build completed successfully. Run the program with ./$OUTPUT_BINARY"
else
    echo "Build failed. Please check the errors above."
    exit 1
fi

################# NEW #########################

#!/bin/bash

# Script to compile the MapReduce project using g++ and generate shared libraries (.so files)

echo "Starting the build process..."

# Ensure g++ is installed
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ is not installed. Please install it and try again."
    exit 1
fi

# Define source files and output binary
SOURCE_FILES="main.cpp mapper.cpp reducer.cpp fileHandler.cpp utils.cpp"
OUTPUT_BINARY="MapReduce"
SHARED_LIBRARY="libMapReduce.so"

# Clean previous builds
if [ -f "$OUTPUT_BINARY" ]; then
    echo "Cleaning previous build..."
    rm -f "$OUTPUT_BINARY"
fi

if [ -f "$SHARED_LIBRARY" ]; then
    echo "Cleaning previous shared library..."
    rm -f "$SHARED_LIBRARY"
fi

# Compile the project into a shared library (.so file)
echo "Compiling source files into a shared library..."
g++ -std=c++17 -shared -fPIC -o "$SHARED_LIBRARY" $SOURCE_FILES -pthread

# Check for success
if [ $? -eq 0 ]; then
    echo "Shared library (.so file) created successfully: $SHARED_LIBRARY"
else
    echo "Build failed for shared library. Please check the errors above."
    exit 1
fi

# Compile the project into an executable binary
echo "Compiling source files into an executable binary..."
g++ -std=c++17 -o "$OUTPUT_BINARY" $SOURCE_FILES -pthread

# Check for success
if [ $? -eq 0 ]; then
    echo "Build completed successfully. Run the program with ./$OUTPUT_BINARY"
else
    echo "Build failed. Please check the errors above."
    exit 1
fi
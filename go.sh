#!/bin/bash

# Script to compile the MapReduce project using g++

echo "Initiating project build..."

# Ensure the C++ compiler is available
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ compiler not found. Please install g++ and try again."
    exit 1
fi

# Define the source files and output binary
SOURCE_FILES="main.cpp mapper.cpp reducer.cpp utils.cpp logger.cpp error_handler.cpp"
OUTPUT_BINARY="mapReduce"

# Clean up any previous builds
if [ -f "$OUTPUT_BINARY" ]; then
    echo "Cleaning up previous build..."
    rm -f "$OUTPUT_BINARY"
fi

# Compile the project
echo "Compiling source files..."
g++ -std=c++17 -o "$OUTPUT_BINARY" $SOURCE_FILES -pthread

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "SUCCESS: Build completed. Run the program with ./$OUTPUT_BINARY"
else
    echo "ERROR: Build failed. Please check the error messages above for details."
    exit 1
fi
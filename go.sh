#!/bin/bash

# Consolidated Script to Compile the MapReduce Project with g++

# Function to check if g++ is installed
check_gpp_installed() {
    if ! command -v g++ &> /dev/null; then
        echo "Error: g++ is not installed. Please install it and try again."
        exit 1
    fi
}

# Function to clean previous builds
clean_previous_builds() {
    local files_to_clean=("$@")
    for file in "${files_to_clean[@]}"; do
        if [ -f "$file" ]; then
            echo "Cleaning previous build: $file"
            rm -f "$file"
        fi
    done
}

# Function to compile the project
compile_project() {
    local output_file=$1
    local compile_flags=$2
    echo "Compiling source files into $output_file..."
    g++ -std=c++17 $compile_flags -o "$output_file" $SOURCE_FILES -pthread

    if [ $? -eq 0 ]; then
        echo "Build successful: $output_file"
    else
        echo "Build failed for $output_file. Please check the errors above."
        exit 1
    fi
}

# Main Script Starts Here
echo "Starting the build process..."

# Check if g++ is installed
check_gpp_installed

# Define source files and output targets
SOURCE_FILES="main.cpp mapper.cpp reducer.cpp fileHandler.cpp utils.cpp"
OUTPUT_BINARY="MapReduce"
SHARED_LIBRARY="libMapReduce.so"

# Clean previous builds
clean_previous_builds "$OUTPUT_BINARY" "$SHARED_LIBRARY"

# Compile shared library
compile_project "$SHARED_LIBRARY" "-shared -fPIC"

# Compile executable binary
compile_project "$OUTPUT_BINARY" ""

echo "Build process completed successfully. You can run the program with ./$OUTPUT_BINARY or use the shared library $SHARED_LIBRARY."
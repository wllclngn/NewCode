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

######################## OLD ##########################################

#!/bin/bash

################################################################################

# DETERMINE THE OPERATING SYSTEM
OS=$(uname -s)

# UNIX-LIKE SYSTEM DETERMINATION
echo "UNIX-like $OS detected, compilation script beginning..."

# DETERMINE boost's VERSION; IF IT EXISTS, DNE
software_boost_version() {
    if [ -f /usr/include/boost/version.hpp ]; then
        local boost_version=$(grep -oP "BOOST_LIB_VERSION\s*\"[0-9_]+\"" /usr/include/boost/version.hpp | grep -oP "[0-9_]+")
        if [ -n "$boost_version" ]; then
            echo "LOG: boost version detected as ${boost_version//_/\.}"
            echo "$boost_version"
            return 0
        fi
    fi
    echo "WARNING: Your system's boost version could not be determined. Please install boost if it is not already."
    return 1
}

# BEGIN MAIN SCRIPT
echo "Initiating project build..."

# DETERMINE boost's VERSION; IF IT EXISTS, DNE
boost_version=$(software_boost_version)
if [ $? -ne 0 ]; then
    echo "ERROR: boost's version cannot be determined. Aborting build..."
    exit 1
fi

# SET boost VERSION DYNAMICALLY BASED ON USER'S SYSTEM
boost_version_formatted=${boost_version//_/.}
BOOST_ROOT="/usr/local/boost_$boost_version"
CPLUS_INCLUDE_PATH="$BOOST_ROOT/include:$CPLUS_INCLUDE_PATH"
LIBRARY_PATH="$BOOST_ROOT/lib:$LIBRARY_PATH"
LD_LIBRARY_PATH="$BOOST_ROOT/lib:$LD_LIBRARY_PATH"

export CPLUS_INCLUDE_PATH
export LIBRARY_PATH
export LD_LIBRARY_PATH

# COMPILE BASED ON DYNAMIC RESULTS
# FUTURE IMPLEMENTATION CODE: g++ -std=c++17 -fPIC -shared -o libmapper.so mapper.cpp
g++ -std=c++17 mapReduce.cpp -o mapReduce -I "$BOOST_ROOT/include" -L "$BOOST_ROOT/lib" -lboost_filesystem -lboost_system -L. -lmapper

if [ $? -eq 0 ]; then
    echo "SUCCESS: Run the build by entering ./mapReduce"
else
    echo "WARNING: The build was unsuccessful. Check for ERRORs above for more detail."
fi

#!/bin/bash

# Test Script for MapReduce Project

echo "Starting final testing..."

# Step 1: Clean and Rebuild
rm -rf build
mkdir build && cd build
cmake ..
make || { echo "Build failed"; exit 1; }

# Step 2: Run Tests
echo "Running tests..."
./utils_test || { echo "Utils tests failed"; exit 1; }
./mapper_test || { echo "Mapper tests failed"; exit 1; }
./reducer_test || { echo "Reducer tests failed"; exit 1; }
./integration_test || { echo "Integration tests failed"; exit 1; }

# Step 3: Manual Test with Sample Data
echo "Running manual test..."
cd ..
mkdir -p sample_input sample_output sample_temp
echo -e "Hello world\nHello again" > sample_input/file1.txt
echo -e "Another test\nHello world" > sample_input/file2.txt

./build/MapReduce <<EOF
sample_input
sample_output
sample_temp
EOF

echo "Manual test completed. Check the output in 'sample_output'."

# Step 4: Verify Output Files
if [ -f "sample_output/output.txt" ] && [ -f "sample_output/output_summed.txt" ]; then
    echo "Output files generated successfully."
else
    echo "Output files missing!"
    exit 1
fi

echo "All tests passed. Project is ready for release!"

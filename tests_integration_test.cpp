#include "../utils.h"
#include "../mapper.h"
#include "../reducer.h"
#include "test_framework.h"
#include <fstream>
#include <string>

TEST_CASE(IntegrationTest) {
    // Step 1: Prepare test data
    std::string inputFolder = "./test_data/input";
    std::string tempFolder = "./test_data/temp";
    std::string outputFolder = "./test_data/output";

    // Create sample input files
    std::ofstream file1(inputFolder + "/file1.txt");
    file1 << "Hello world\nHello again";
    file1.close();

    std::ofstream file2(inputFolder + "/file2.txt");
    file2 << "Another test\nHello world";
    file2.close();

    // Step 2: Map phase
    Mapper mapper;
    mapper.map_words({"Hello world", "Hello again"}, tempFolder);

    // Step 3: Reduce phase
    Reducer reducer;
    reducer.reduce(mapper.get_mapped_data());

    // Step 4: Verify output
    auto reducedData = reducer.get_reduced_data();
    ASSERT_EQ(3, reducedData["hello"].size());
    ASSERT_EQ(1, reducedData["world"].size());
}
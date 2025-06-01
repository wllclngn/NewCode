#include "../mapper.h"
#include "test_framework.h"
#include <vector>
#include <string>

TEST_CASE(MapperTests) {
    // Test clean_word
    ASSERT_EQ("hello", Mapper::clean_word("Hello"));
    ASSERT_EQ("123hello", Mapper::clean_word("123Hello!"));
    ASSERT_EQ("", Mapper::clean_word("!@#$%^&*"));

    // Test map_words
    Mapper mapper;
    std::vector<std::string> lines = {"This is a test", "Another test"};
    std::string tempFolderPath = "./temp";
    mapper.map_words(lines, tempFolderPath);

    // Verify output file exists
    std::ifstream file(tempFolderPath + "/mapped_temp.txt");
    ASSERT_TRUE(file.is_open());

    // Verify contents
    std::string line;
    std::getline(file, line);
    ASSERT_EQ("<\"this\", 1>", line);
    std::getline(file, line);
    ASSERT_EQ("<\"is\", 1>", line);
    file.close();
}

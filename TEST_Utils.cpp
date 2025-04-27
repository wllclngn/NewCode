#include "../utils.h"
#include "test_framework.h"

TEST_CASE(UtilsTests) {
    // Test isValidDirectory
    ASSERT_TRUE(Utils::isValidDirectory("./"));  // Current directory should exist
    ASSERT_FALSE(Utils::isValidDirectory("./nonexistent"));  // Nonexistent directory

    // Test removeTrailingSlash
    std::string pathWithSlash = "/path/to/directory/";
    Utils::removeTrailingSlash(pathWithSlash);
    ASSERT_EQ("/path/to/directory", pathWithSlash);

    std::string pathWithoutSlash = "/path/to/directory";
    Utils::removeTrailingSlash(pathWithoutSlash);
    ASSERT_EQ("/path/to/directory", pathWithoutSlash);
}

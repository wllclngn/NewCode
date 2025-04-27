#pragma once

#include <string>

namespace Utils {

    // Validates if a folder path exists and is a directory.
    bool isValidDirectory(const std::string& folderPath);

    // Removes trailing slashes or backslashes from a folder path.
    void removeTrailingSlash(std::string& folderPath);

    // Prompts the user for a folder path and validates it.
    bool getUserInputFolderPath(const std::string& promptMessage, std::string& folderPath);

} // namespace Utils
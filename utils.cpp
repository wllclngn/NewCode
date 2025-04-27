#include "utils.h"
#include <iostream>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace Utils {

    bool isValidDirectory(const std::string& folderPath) {
        return fs::exists(folderPath) && fs::is_directory(folderPath);
    }

    void removeTrailingSlash(std::string& folderPath) {
        if (!folderPath.empty() && 
            (folderPath.back() == '/' || folderPath.back() == '\\')) {
            folderPath.pop_back();
        }
    }

    bool getUserInputFolderPath(const std::string& promptMessage, std::string& folderPath) {
        std::cout << promptMessage;
        std::getline(std::cin, folderPath);

        if (folderPath.empty()) {
            std::cerr << "No folder path entered. Exiting." << std::endl;
            return false;
        }

        removeTrailingSlash(folderPath);

        if (!isValidDirectory(folderPath)) {
            std::cerr << "Folder does not exist or is not a directory. Exiting." << std::endl;
            return false;
        }

        return true;
    }

} // namespace Utils
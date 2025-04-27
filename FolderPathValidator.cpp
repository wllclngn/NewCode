#include <iostream>
#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// Function to validate folder path with reprompt
std::string validateFolderPath(const std::string &folderType)
{
    std::string path;

    while (true)
    {
        std::cout << "Enter the folder path for the " << folderType << " directory: ";
        std::getline(std::cin, path);

        // Check if user entered a valid path
        if (path.empty())
        {
            std::cerr << "No " << folderType << " folder path entered. Please try again." << std::endl;
            continue;
        }

        // Remove trailing slash or backslash if present
        if (!path.empty() && (path.back() == '/' || path.back() == '\\'))
        {
            path.pop_back();
        }

        // Check if the path exists and is a directory
        if (fs::exists(path) && fs::is_directory(path))
        {
            return path; // Return the valid path
        }
        else
        {
            std::cerr << folderType << " folder does not exist or is not a directory. Please try again." << std::endl;
        }
    }
}
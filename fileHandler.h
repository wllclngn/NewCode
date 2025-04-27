#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

/*
#include "utils.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

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

}

///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <filesystem>

namespace Utils {
    bool is_valid_directory(const std::string& path) {
        return std::filesystem::exists(path) && std::filesystem::is_directory(path);
    }

    void remove_trailing_slash(std::string& path) {
        if (!path.empty() && (path.back() == '/' || path.back() == '\\')) {
            path.pop_back();
        }
    }
}

*/

class FileHandler {
public:
    // Reads a file and stores its lines in a vector
    static bool read_file(const std::string &filename, std::vector<std::string> &lines) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << " for reading.\n";
            return false;
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        return true;
    }

    // Validates input directory existence
    static bool validate_directory(const std::string &folder_path) {
        if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
            std::cerr << "Error: Directory " << folder_path << " does not exist or is not valid.\n";
            return false;
        }
        return true;
    }

    // Writes filenames in a directory to a specified output file
    static bool write_filenames_to_file(const std::string &folder_path, const std::string &output_filename) {
        std::ofstream outfile(output_filename);
        if (!outfile) {
            std::cerr << "Error: Could not open " << output_filename << " for writing.\n";
            return false;
        }
        for (const auto &entry : fs::directory_iterator(folder_path)) {
            if (entry.is_regular_file()) {
                outfile << entry.path().filename().string() << "\n";
            }
        }
        outfile.close();
        return true;
    }

    // Writes key-value pairs to an output file
    static bool write_output(const std::string &filename, const std::map<std::string, int> &data) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << " for writing.\n";
            return false;
        }
        for (const auto &kv : data) {
            file << kv.first << ": " << kv.second << "\n";
        }
        file.close();
        return true;
    }
};
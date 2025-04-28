#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "ERROR_Handler.h"

namespace fs = std::filesystem;

class FileHandler {
public:
    static bool read_file(const std::string &filename, std::vector<std::string> &lines) {
        std::ifstream file(filename);
        if (!file) {
            ErrorHandler::reportError("Could not open file " + filename + " for reading.");
            return false;
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        return true;
    }

    static bool validate_directory(const std::string &folder_path) {
        if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
            ErrorHandler::reportError("Directory " + folder_path + " does not exist or is not valid.");
            return false;
        }
        return true;
    }

    static bool write_filenames_to_file(const std::string &folder_path, const std::string &output_filename) {
        std::ofstream outfile(output_filename);
        if (!outfile) {
            ErrorHandler::reportError("Could not open " + output_filename + " for writing.");
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

    static bool write_output(const std::string &filename, const std::map<std::string, int> &data) {
        std::ofstream file(filename);
        if (!file) {
            ErrorHandler::reportError("Could not open file " + filename + " for writing.");
            return false;
        }
        for (const auto &kv : data) {
            file << kv.first << ": " << kv.second << "\n";
        }
        file.close();
        return true;
    }
};
#pragma once // Use #pragma once for modern compilers
#ifndef FILE_HANDLER_H // Traditional include guards for wider compatibility
#define FILE_HANDLER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>     // For write_output, write_summed_output, read_mapped_data
#include <sstream> // For read_mapped_data
#include <algorithm> // For std::remove for trim

#include "ERROR_Handler.h" // Assuming ErrorHandler.h is accessible
#include "Logger.h"      // Assuming Logger.h is accessible

namespace fs = std::filesystem;

class FileHandler {
public:
    // Reads all lines from a file into a vector of strings.
    static bool read_file(const std::string &filename, std::vector<std::string> &lines) {
        std::ifstream file(filename);
        if (!file) {
            ErrorHandler::reportError("FILE_HANDLER: Could not open file " + filename + " for reading.");
            return false;
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        return true;
    }

    // Original validate_directory (4 arguments) - used by interactive mode
    static bool validate_directory(std::string &folder_path, 
                                   std::vector<std::string> &file_paths, 
                                   const std::string &default_path_if_empty,
                                   bool create_if_missing) {
        Logger &logger = Logger::getInstance();
        logger.log("FILE_HANDLER (4-arg): Validating dir. User Path: '" + folder_path + "', Default: '" + default_path_if_empty + "'");

        bool path_was_empty_and_default_used = false;
        if (folder_path.empty()) {
            if (!default_path_if_empty.empty()) {
                folder_path = default_path_if_empty;
                path_was_empty_and_default_used = true;
                logger.log("FILE_HANDLER (4-arg): User path empty, using default: " + folder_path);
            } else {
                logger.log("FILE_HANDLER (4-arg): User path and default path are empty. Cannot validate.");
                ErrorHandler::reportError("FILE_HANDLER (4-arg): Directory path and default path cannot both be empty.");
                return false;
            }
        }

        if (fs::exists(folder_path)) {
            if (fs::is_directory(folder_path)) {
                logger.log("FILE_HANDLER (4-arg): Validated existing directory: " + folder_path);
                // Populate file_paths only if this is likely an input directory.
                // Heuristic: if the path was NOT created using a default output/temp folder name.
                if (!path_was_empty_and_default_used || default_path_if_empty.find("outputFolder") == std::string::npos && default_path_if_empty.find("tempFolder") == std::string::npos) {
                    try {
                        file_paths.clear(); 
                        for (const auto &entry : fs::directory_iterator(folder_path)) {
                            if (entry.is_regular_file()) {
                                std::string current_file_path = entry.path().string();
                                if (entry.path().extension() == ".txt") {
                                    file_paths.push_back(current_file_path);
                                } else {
                                    logger.log("FILE_HANDLER (4-arg): Skipped non-txt file: " + current_file_path);
                                }
                            }
                        }
                        logger.log("FILE_HANDLER (4-arg): " + std::to_string(file_paths.size()) + " .txt files found in: " + folder_path);
                    } catch (const fs::filesystem_error &e) {
                        ErrorHandler::reportError("FILE_HANDLER (4-arg): Error retrieving file paths from " + folder_path + ": " + e.what());
                        return false;
                    }
                }
                return true;
            } else {
                ErrorHandler::reportError("FILE_HANDLER (4-arg): Path exists but is not a directory: " + folder_path);
                return false;
            }
        } else if (create_if_missing) {
            logger.log("FILE_HANDLER (4-arg): Directory does not exist, attempting to create: " + folder_path);
            try {
                if (fs::create_directories(folder_path)) {
                    logger.log("FILE_HANDLER (4-arg): Directory created successfully: " + folder_path);
                    return true;
                } else {
                    ErrorHandler::reportError("FILE_HANDLER (4-arg): Failed to create directory (fs::create_directories returned false): " + folder_path);
                    return false;
                }
            } catch (const fs::filesystem_error &e) {
                ErrorHandler::reportError("FILE_HANDLER (4-arg): Filesystem error while creating directory " + folder_path + ": " + e.what());
                return false;
            }
        } else {
            ErrorHandler::reportError("FILE_HANDLER (4-arg): Directory does not exist and creation is disabled: " + folder_path);
            return false;
        }
    }

    // New validate_directory (2 arguments) - for command-line controller
    static bool validate_directory(const std::string& path, bool create_if_missing) {
        Logger::getInstance().log("FILE_HANDLER (2-arg): Validating directory. Path: '" + path + "'");
        if (path.empty()){
            ErrorHandler::reportError("FILE_HANDLER (2-arg): Directory path cannot be empty.");
            return false;
        }

        if (fs::exists(path)) {
            if (fs::is_directory(path)) {
                Logger::getInstance().log("FILE_HANDLER (2-arg): Validated existing directory: " + path);
                return true;
            } else {
                ErrorHandler::reportError("FILE_HANDLER (2-arg): Path exists but is not a directory: " + path);
                return false;
            }
        } else if (create_if_missing) {
            Logger::getInstance().log("FILE_HANDLER (2-arg): Directory does not exist, attempting to create: " + path);
            try {
                if (fs::create_directories(path)) { 
                    Logger::getInstance().log("FILE_HANDLER (2-arg): Directory created successfully: " + path);
                    return true;
                } else {
                    ErrorHandler::reportError("FILE_HANDLER (2-arg): Failed to create directory (fs::create_directories returned false): " + path);
                    return false;
                }
            } catch (const fs::filesystem_error& e) {
                ErrorHandler::reportError("FILE_HANDLER (2-arg): Filesystem error while creating directory " + path + ": " + e.what());
                return false;
            }
        } else {
            ErrorHandler::reportError("FILE_HANDLER (2-arg): Directory does not exist and creation is disabled: " + path);
            return false;
        }
    }

    // New get_files_in_directory - for command-line controller
    static bool get_files_in_directory(const std::string& dirPath, 
                                       std::vector<std::string>& filePaths, 
                                       const std::string& extensionFilter = "") { 
        Logger::getInstance().log("FILE_HANDLER: Getting files from dir: " + dirPath + (extensionFilter.empty() ? "" : " (Filter: " + extensionFilter + ")"));
        filePaths.clear(); 

        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            ErrorHandler::reportError("FILE_HANDLER: Cannot get files, path is not a valid directory: " + dirPath);
            return false;
        }

        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.is_regular_file()) {
                    if (extensionFilter.empty() || entry.path().extension().string() == extensionFilter) {
                        filePaths.push_back(entry.path().string());
                    }
                }
            }
            Logger::getInstance().log("FILE_HANDLER: Found " + std::to_string(filePaths.size()) + " files with filter '" + extensionFilter + "' in " + dirPath);
            return true;
        } catch (const fs::filesystem_error& e) {
            ErrorHandler::reportError("FILE_HANDLER: Error iterating directory " + dirPath + ": " + e.what());
            return false;
        }
    }
    
    static bool create_empty_file(const std::string& filepath) {
        Logger::getInstance().log("FILE_HANDLER: Creating empty file: " + filepath);
        std::ofstream file(filepath);
        if (!file.is_open()) {
            ErrorHandler::reportError("FILE_HANDLER: Failed to create empty file: " + filepath);
            return false;
        }
        file.close(); // Ensures file is created on disk
        return true;
    }

    static bool write_output(const std::string &filename, const std::map<std::string, int> &data) {
        if (data.empty()) {
            Logger::getInstance().log("FILE_HANDLER: WARNING - Data for write_output is empty. Output file " + filename + " will be empty.");
        }

        std::ofstream file(filename);
        if (!file) {
            ErrorHandler::reportError("FILE_HANDLER: Could not open file " + filename + " for writing.");
            return false;
        }
        for (const auto &kv : data) {
            file << kv.first << ": " << kv.second << "\n";
        }
        file.close();
        Logger::getInstance().log("FILE_HANDLER: Successfully wrote " + std::to_string(data.size()) + " entries to " + filename);
        return true;
    }

    static bool write_summed_output(const std::string &filename, const std::map<std::string, std::vector<int>> &data) {
        if (data.empty()) {
            Logger::getInstance().log("FILE_HANDLER: WARNING - Data for write_summed_output is empty. Output file " + filename + " will be empty.");
        }

        std::ofstream outfile(filename);
        if (!outfile) {
            ErrorHandler::reportError("FILE_HANDLER: Could not open file " + filename + " for writing (summed output).");
            return false;
        }
        for (const auto &kv : data) {
            long long sum = 0; // Use long long for sum to avoid overflow if counts are large
            for (int count : kv.second) {
                sum += count;
            }
            outfile << "<\"" << kv.first << "\", " << sum << ">\n";
        }
        outfile.close();
        Logger::getInstance().log("FILE_HANDLER: Successfully wrote summed output to " + filename);
        return true;
    }
    
    // Helper to trim whitespace from both ends of a string
    static std::string trim_string(const std::string& str) {
        std::string s = str;
        // Trim leading whitespace
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // Trim trailing whitespace
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
        return s;
    }

    static bool read_mapped_data(const std::string &filename, std::vector<std::pair<std::string, int>> &mapped_data) {
        Logger::getInstance().log("FILE_HANDLER: Attempting to read mapped data from file: " + filename);
    
        std::ifstream infile(filename);
        if (!infile) {
            ErrorHandler::reportError("FILE_HANDLER: Could not open file " + filename + " for reading (mapped data).");
            return false;
        }
    
        std::string line;
        int line_number = 0;
        while (std::getline(infile, line)) {
            line_number++;
    
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos && colon_pos > 0) { // Ensure colon is not the first character
                std::string word = line.substr(0, colon_pos);
                std::string count_str = line.substr(colon_pos + 1);
    
                word = trim_string(word);
                count_str = trim_string(count_str);
    
                if (word.empty()) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Skipped line " + std::to_string(line_number) + " in " + filename + " (empty word after trim): " + line);
                    continue;
                }
                if (count_str.empty()) {
                     Logger::getInstance().log("FILE_HANDLER: WARNING - Skipped line " + std::to_string(line_number) + " in " + filename + " (empty count string after trim): " + line);
                    continue;
                }

                try {
                    int count = std::stoi(count_str);
                    mapped_data.emplace_back(word, count);
                } catch (const std::invalid_argument& ia) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Invalid number format on line " + std::to_string(line_number) + " in " + filename + " (count: '" + count_str + "'): " + line);
                } catch (const std::out_of_range& oor) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Number out of range on line " + std::to_string(line_number) + " in " + filename + " (count: '" + count_str + "'): " + line);
                }
            } else {
                std::string trimmed_line = trim_string(line);
                if (!trimmed_line.empty()) { // Only log warning for non-empty malformed lines
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Skipped malformed line " + std::to_string(line_number) + " in " + filename + " (format error): " + line);
                }
            }
        }
        infile.close();
    
        if (mapped_data.empty() && line_number > 0) { // If file had lines but no valid data was parsed
            Logger::getInstance().log("FILE_HANDLER: WARNING - No valid data parsed from file: " + filename + " (" + std::to_string(line_number) + " lines read).");
        } else {
            Logger::getInstance().log("FILE_HANDLER: Successfully read " + std::to_string(mapped_data.size()) + " entries from file: " + filename);
        }
        return true;
    }
};

#endif // FILE_HANDLER_H
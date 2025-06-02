#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <sstream> // Required for std::stringstream
#include "ERROR_Handler.h"
#include "Logger.h"

namespace fs = std::filesystem;
class FileHandler {
public:
    static std::ifstream ia;
    static std::ofstream oor;

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

    static bool validate_directory(std::string &folder_path, std::vector<std::string> &file_paths, const std::string &input_dir, bool create_if_missing = true) {
        Logger &logger = Logger::getInstance();
        logger.log("Starting directory validation process.");

        // DIRECTORY VALIDATION: EXISTS, DNE
        if (fs::exists(folder_path) && input_dir.empty()) {
            if (fs::is_directory(folder_path)) {
                logger.log("Validated directory: " + folder_path);
                std::cout << "LOG: The directory " << folder_path << " already exists. Proceeding..." << std::endl;

                // GET FILE PATHS FROM USER INPUT
                try {
                    for (const auto &entry : fs::directory_iterator(folder_path)) {
                        if (entry.is_regular_file()) {
                            std::string file_path = entry.path().string();
                            if (entry.path().extension() == ".txt") {
                                file_paths.push_back(file_path);
                            } else {
                                logger.log("Skipped non-txt file: " + file_path);
                            }
                        }
                    }
                    logger.log("File paths successfully retrieved from directory: " + folder_path);
                } catch (const fs::filesystem_error &e) {
                    ErrorHandler::reportError("Could not retrieve file paths from folder: " + folder_path + ". Error: " + e.what());
                    return false;
                }

                return true;
            } else {
                logger.log("ERROR: Path exists but is not a directory: " + folder_path);
                return false;
            }
        }
        
        // IF A USER HAS INPUT BLANK outputFolder AND/OR tempFolder, CREATE DIR BASED ON INPUT FOLDER
        if (folder_path.empty() && !input_dir.empty() && create_if_missing) {
            logger.log("Folder path is blank. Deriving from original input folder: " + input_dir);
            try {
                // Attempt to create the directory. If it already exists, this might not be an error,
                // but fs::create_directory returns false if it already exists.
                // We should ensure the path is usable.
                if (!fs::exists(input_dir)) {
                    if (fs::create_directory(input_dir)) {
                        logger.log("Directory created successfully: " + input_dir);
                    } else {
                        logger.log("ERROR: Failed to create directory: " + input_dir);
                        return false;
                    }
                } else if (!fs::is_directory(input_dir)) {
                     logger.log("ERROR: Path exists but is not a directory: " + input_dir);
                     return false;
                }
                folder_path = input_dir; // Set folder_path regardless of whether it was just created or already existed
                logger.log("Using directory: " + folder_path);
                return true;

            } catch (const fs::filesystem_error &e) {
                logger.log("Filesystem error when creating/using directory " + input_dir + ": " + std::string(e.what()));
                return false;
            }
        }
        
        // IF USER SUPPLIED DIRECTORY, BUT DNE, CREATE BASED ON PATH
        // The condition `!folder_path.empty() || !create_if_missing` seems a bit off.
        // It should likely be `!folder_path.empty() && create_if_missing` for creating a specified non-existent path.
        // Or if it's just about ensuring a non-empty path exists (and not creating it if create_if_missing is false):
        if (!folder_path.empty()) {
            if (!fs::exists(folder_path)) {
                if (create_if_missing) {
                    try {
                        if (fs::create_directory(folder_path)) {
                            logger.log("Directory created successfully: " + folder_path);
                            // No need to set folder_path = input_dir here, folder_path is already what we want.
                        } else {
                            logger.log("ERROR: Failed to create directory (fs::create_directory returned false): " + folder_path);
                            return false;
                        }
                    } catch (const fs::filesystem_error &e) {
                        logger.log("Filesystem error when creating directory " + folder_path + ": " + std::string(e.what()));
                        return false;
                    }
                } else {
                    logger.log("Directory does not exist and creation is disabled: " + folder_path);
                    return false;
                }
            } else if (!fs::is_directory(folder_path)) {
                logger.log("ERROR: Path exists but is not a directory: " + folder_path);
                return false;
            }
            // If we reach here, folder_path exists and is a directory (or was just created)
             // GET FILE PATHS FROM USER INPUT (similar to the first block, in case the directory was just created)
            try {
                file_paths.clear(); // Clear any previous paths if validating again
                for (const auto &entry : fs::directory_iterator(folder_path)) {
                    if (entry.is_regular_file()) {
                        std::string file_path_str = entry.path().string();
                        if (entry.path().extension() == ".txt") {
                            file_paths.push_back(file_path_str);
                        } else {
                            logger.log("Skipped non-txt file: " + file_path_str);
                        }
                    }
                }
                logger.log("File paths successfully retrieved from directory: " + folder_path);
            } catch (const fs::filesystem_error &e) {
                ErrorHandler::reportError("Could not retrieve file paths from folder: " + folder_path + ". Error: " + e.what());
                // Return false if we can't list files, even if dir exists/was created.
                return false; 
            }
            return true;
        } else { // folder_path is empty and (input_dir is empty or create_if_missing is false)
             if (folder_path.empty() && !create_if_missing) {
                 logger.log("Folder path is empty and creation is disabled.");
                 return false;
             }
             // This case might indicate an issue or an unhandled scenario if folder_path is empty
             // and we are not deriving from input_dir.
             logger.log("Folder path is empty and not derived from input directory. Validation cannot proceed for file listing.");
             return false; // Or true if an empty folder_path that wasn't created is acceptable. Depends on desired logic.
        }
    }

    static bool write_output(const std::string &filename, const std::map<std::string, int> &data) {
        if (data.empty()) {
            Logger::getInstance().log("WARNING: Data is empty. Output file will be empty.");
        }

        std::ofstream file(filename);
        if (!file) {
            ErrorHandler::reportError("Could not open file " + filename + " for writing. Check permissions or directory existence.");
            return false;
        }
        for (const auto &kv : data) {
            file << kv.first << ": " << kv.second << "\n";
        }
        file.close();
        if (file.fail()) { // Check for errors after closing
            ErrorHandler::reportError("Failed to properly write or close file: " + filename);
            return false;
        }
        return true;
    }

    static bool write_summed_output(const std::string &filename, const std::map<std::string, std::vector<int>> &data) {
        if (data.empty()) {
            Logger::getInstance().log("WARNING: Data is empty. Output file will be empty.");
        }

        std::ofstream outfile(filename);
        if (!outfile) {
            ErrorHandler::reportError("Could not open file " + filename + " for writing. Check permissions or directory existence.");
            return false;
        }
        for (const auto &kv : data) {
            long long sum = 0; // Use long long for sum to avoid overflow if counts are large
            for (int count : kv.second) {
                sum += count;
            }

#ifdef DEBUG
            // This will print to cout if DEBUG is defined during compilation
            std::cout << "ENTRY, write_summed_output: " << kv.first << "." << std::endl;
#endif
            outfile << "<\"" << kv.first << "\", " << sum << ">\n";
        }
        outfile.close();
        if (outfile.fail()) { // Check for errors after closing
            ErrorHandler::reportError("Failed to properly write or close file: " + filename);
            return false;
        }
        return true;
    }

    static bool read_mapped_data(const std::string &filename, std::vector<std::pair<std::string, int>> &mapped_data) {
        Logger::getInstance().log("Attempting to read mapped data from file: " + filename);
    
        std::ifstream infile(filename);
        if (!infile) {
            ErrorHandler::reportError("Could not open file " + filename + " for reading.");
            return false;
        }
    
        std::string line;
        int line_number = 0;
        while (std::getline(infile, line)) {
            line_number++;
            //Logger::getInstance().log("Processing line " + std::to_string(line_number) + ": " + line);
    
            // Parse the line in `word\tcount` format
            size_t tab_pos = line.find('\t'); // Changed from colon to tab
            if (tab_pos != std::string::npos) {
                try {
                    std::string word = line.substr(0, tab_pos);
                    std::string count_str = line.substr(tab_pos + 1);
    
                    // Trim whitespace from word and count
                    // Leading whitespace
                    word.erase(0, word.find_first_not_of(" \t\n\r\f\v"));
                    // Trailing whitespace
                    word.erase(word.find_last_not_of(" \t\n\r\f\v") + 1);
                    
                    // Leading whitespace
                    count_str.erase(0, count_str.find_first_not_of(" \t\n\r\f\v"));
                    // Trailing whitespace
                    count_str.erase(count_str.find_last_not_of(" \t\n\r\f\v") + 1);
    
                    int count = 0;
                    // Use std::stoi for robust conversion, check for errors
                    if (!count_str.empty()) { // Ensure count_str is not empty before trying to convert
                        try {
                            count = std::stoi(count_str);
                            if (!word.empty()) { // Ensure word is not empty after trimming
                                //Logger::getInstance().log("Parsed entry: " + word + " -> " + std::to_string(count));
                                mapped_data.emplace_back(word, count);
                            } else {
                                Logger::getInstance().log("WARNING: Word became empty after trimming on line " + std::to_string(line_number) + ": " + line);
                            }
                        } catch (const std::invalid_argument& ia) {
                            Logger::getInstance().log("WARNING: Invalid number format for count on line " + std::to_string(line_number) + " ('" + count_str + "'): " + line);
                        } catch (const std::out_of_range& oor) {
                            Logger::getInstance().log("WARNING: Count out of range on line " + std::to_string(line_number) + " ('" + count_str + "'): " + line);
                        }
                    } else {
                         Logger::getInstance().log("WARNING: Empty count string on line " + std::to_string(line_number) + ": " + line);
                    }
                } catch (const std::exception &e) { // Catch other potential exceptions like std::out_of_range from substr
                    Logger::getInstance().log("ERROR: Exception while processing line " + std::to_string(line_number) + ": " + line + ". Exception: " + e.what());
                }
            } else {
                // Log if line is not empty but no tab was found
                if (!line.empty() && line.find_first_not_of(" \t\n\r\f\v") != std::string::npos) { // Check if line is not just whitespace
                   Logger::getInstance().log("WARNING: Skipped malformed line (no tab separator found) " + std::to_string(line_number) + ": " + line);
                }
            }
        }
    
        infile.close();
    
        if (mapped_data.empty() && line_number > 0) { // Also check if any lines were processed
            Logger::getInstance().log("WARNING: No valid data found in file: " + filename + " after processing " + std::to_string(line_number) + " lines.");
        } else if (!mapped_data.empty()) {
            Logger::getInstance().log("Successfully read " + std::to_string(mapped_data.size()) + " entries from file: " + filename);
        } else {
             // File might be empty or contain only whitespace lines
            Logger::getInstance().log("File was empty or contained no processable data: " + filename);
        }
    
        return true;
    }
};
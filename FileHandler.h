#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "ERROR_Handler.h"
#include "Logger.h"

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

    static bool validate_directory(std::string &folder_path, std::vector<std::string> &file_paths, std::string &input_dir, bool create_if_missing = true) {
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
                if (fs::create_directory(input_dir)) {
                    folder_path = input_dir;
                    logger.log("Directory created successfully: " + input_dir);
                    return true;
                } else {
                    logger.log("ERROR: Failed to create directory: " + input_dir);
                    return false;
                }
            } catch (const fs::filesystem_error &e) {
                logger.log("Filesystem error: " + std::string(e.what()));
                return false;
            }

        }
        
        // IF USER SUPPLIED DIRECTORY, BUT DNE, CREATE BASED ON PATH
        if (!folder_path.empty() || !create_if_missing) {       
            try {
                if (fs::create_directory(folder_path)) {
                    folder_path = input_dir;
                    logger.log("Directory created successfully: " + folder_path);
                    return true;
                } else {
                    logger.log("ERROR: Failed to create directory: " + folder_path);
                    return false;
                }
            } catch (const fs::filesystem_error &e) {
                logger.log("Filesystem error: " + std::string(e.what()));
                return false;
            }

        } else {
            logger.log("Directory does not exist and creation is disabled: " + folder_path);
            return false;
        }
    }

    static bool copy_dlls(const std::string &source_folder, const std::string &target_folder) {
        Logger &logger = Logger::getInstance();
        logger.log("Starting DLL copy process.");

        try {
            if (!fs::exists(target_folder)) {
                fs::create_directories(target_folder);
                logger.log("Target folder created: " + target_folder);
            }

            for (const auto &entry : fs::directory_iterator(source_folder)) {
                if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                    std::string source_path = entry.path().string();
                    std::string target_path = target_folder + "/" + entry.path().filename().string();

                    fs::copy_file(source_path, target_path, fs::copy_options::overwrite_existing);
                    logger.log("Copied DLL: " + source_path + " to " + target_path);
                }
            }
        } catch (const fs::filesystem_error &e) {
            ErrorHandler::reportError("Failed to copy DLL files. Error: " + std::string(e.what()));
            return false;
        }

        logger.log("DLL copy process completed successfully.");
        return true;
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
            int sum = 0;
            for (int count : kv.second) {
                sum += count;
            }

#ifdef DEBUG
            std::cout << "ENTRY, write_summed_output: " << kv.first << "." << std::endl;
#endif
            outfile << "<\"" << kv.first << "\", " << sum << ">\n";
        }
        outfile.close();
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
    
            // Parse the line in `word: count` format
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                try {
                    std::string word = line.substr(0, colon);
                    std::string count_str = line.substr(colon + 1);
    
                    // Trim whitespace from word and count
                    word.erase(0, word.find_first_not_of(" \t"));
                    word.erase(word.find_last_not_of(" \t") + 1);
                    count_str.erase(0, count_str.find_first_not_of(" \t"));
                    count_str.erase(count_str.find_last_not_of(" \t") + 1);
    
                    int count = 0;
                    std::stringstream ss(count_str);
                    ss >> count;
    
                    if (!word.empty() && !ss.fail()) {
                        //Logger::getInstance().log("Parsed entry: " + word + " -> " + std::to_string(count));
                        mapped_data.emplace_back(word, count);
                    } else {
                        //Logger::getInstance().log("WARNING: Invalid data on line " + std::to_string(line_number) + ": " + line);
                    }
                } catch (const std::exception &e) {
                    Logger::getInstance().log("ERROR: Exception while processing line " + std::to_string(line_number) + ": " + line + ". Exception: " + e.what());
                }
            } else {
                Logger::getInstance().log("WARNING: Skipped malformed line " + std::to_string(line_number) + ": " + line);
            }
        }
    
        infile.close();
    
        if (mapped_data.empty()) {
            Logger::getInstance().log("WARNING: No valid data found in file: " + filename);
        } else {
            Logger::getInstance().log("Successfully read " + std::to_string(mapped_data.size()) + " entries from file: " + filename);
        }
    
        return true;
    }
};
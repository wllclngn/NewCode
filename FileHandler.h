#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>      // Added for write_output and read_mapped_data
#include <sstream>  // Added for read_mapped_data
#include <algorithm> // Added for sorting in get_files_in_directory (optional)

#include "ERROR_Handler.h"
#include "Logger.h"

namespace fs = std::filesystem;

class FileHandler {
public:
    static bool read_file(const std::string &filename, std::vector<std::string> &lines) {
        std::ifstream file(filename);
        if (!file) {
            ErrorHandler::reportError("FileHandler: Could not open file " + filename + " for reading.");
            return false;
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        Logger::getInstance().log("FileHandler: Successfully read " + std::to_string(lines.size()) + " lines from " + filename);
        return true;
    }

    // Updated to handle directory creation more explicitly and log actions
    static bool validate_directory(const std::string &folder_path, bool create_if_missing = true) {
        Logger &logger = Logger::getInstance();
        logger.log("FileHandler: Validating directory: " + folder_path);

        if (fs::exists(folder_path)) {
            if (fs::is_directory(folder_path)) {
                logger.log("FileHandler: Directory " + folder_path + " exists and is valid.");
                return true;
            } else {
                ErrorHandler::reportError("FileHandler: Path " + folder_path + " exists but is not a directory.", true);
                return false;
            }
        } else {
            if (create_if_missing) {
                logger.log("FileHandler: Directory " + folder_path + " does not exist. Attempting to create.");
                try {
                    if (fs::create_directories(folder_path)) { // create_directories handles nested paths
                        logger.log("FileHandler: Directory " + folder_path + " created successfully.");
                        return true;
                    } else {
                        // This case might be rare if create_directories fails without an exception,
                        // but good to have a fallback error.
                        ErrorHandler::reportError("FileHandler: Failed to create directory " + folder_path + " (unknown reason).", true);
                        return false;
                    }
                } catch (const fs::filesystem_error &e) {
                    ErrorHandler::reportError("FileHandler: Filesystem error while creating directory " + folder_path + ": " + e.what(), true);
                    return false;
                }
            } else {
                ErrorHandler::reportError("FileHandler: Directory " + folder_path + " does not exist and creation is disabled.");
                return false; // Not necessarily critical, depends on context, but for validate_directory it's a failure.
            }
        }
    }
    
    // Overload for the original validate_directory if still needed by other parts of existing code
    // This version was more complex and tied to specific input_dir logic.
    // For Phase 3, the simpler version above is likely more generally useful.
    // If this specific logic is still critical, it should be refactored or used carefully.
    // For now, I'm keeping the simpler one as the primary.
    /*
    static bool validate_directory(std::string &folder_path, std::vector<std::string> &file_paths, std::string &input_dir, bool create_if_missing = true) {
        // ... original complex logic ...
        // This version should be reviewed if it's still essential for Phase 2 compatibility vs. Phase 3 needs.
        // The simpler validate_directory(const std::string&, bool) is preferred for new Phase 3 controller logic.
        ErrorHandler::reportError("FileHandler: Deprecated validate_directory overload called. Please refactor.", true);
        return false;
    }
    */


    // New function to get files from a directory, optionally filtering by extension
    static bool get_files_in_directory(
        const std::string& dir_path, 
        std::vector<std::string>& file_paths, 
        const std::string& extension_filter = "" // e.g., ".txt", empty means all files
    ) {
        Logger::getInstance().log("FileHandler: Getting files from directory: " + dir_path + (extension_filter.empty() ? "" : " with extension " + extension_filter));
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            ErrorHandler::reportError("FileHandler: Directory " + dir_path + " does not exist or is not a directory.");
            return false;
        }

        file_paths.clear();
        try {
            for (const auto &entry : fs::directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    if (extension_filter.empty() || entry.path().extension() == extension_filter) {
                        file_paths.push_back(entry.path().string());
                    }
                }
            }
            // Optional: Sort file paths for deterministic behavior, especially for mappers
            std::sort(file_paths.begin(), file_paths.end());
            Logger::getInstance().log("FileHandler: Found " + std::to_string(file_paths.size()) + " files in " + dir_path);
            return true;
        } catch (const fs::filesystem_error &e) {
            ErrorHandler::reportError("FileHandler: Could not retrieve file paths from directory " + dir_path + ". Error: " + e.what());
            return false;
        }
    }


    static bool write_output(const std::string &filename, const std::map<std::string, int> &data) {
        Logger::getInstance().log("FileHandler: Writing output to file: " + filename);
        if (data.empty()) {
            Logger::getInstance().log("FileHandler: WARNING - Data is empty. Output file " + filename + " will be empty.");
        }

        std::ofstream file(filename);
        if (!file) {
            ErrorHandler::reportError("FileHandler: Could not open file " + filename + " for writing. Check permissions or directory existence.");
            return false;
        }
        for (const auto &kv : data) {
            file << kv.first << ": " << kv.second << "\n";
        }
        file.close();
        Logger::getInstance().log("FileHandler: Successfully wrote " + std::to_string(data.size()) + " entries to " + filename);
        return true;
    }

    // This function might be useful for the controller's final aggregation step if it needs to sum up
    // values from different reducer outputs. However, if reducers already produce final sums for their keys,
    // and keys are perfectly partitioned, this might not be needed.
    // The `write_output` is more general for key:value pairs.
    static bool write_summed_output(const std::string &filename, const std::map<std::string, std::vector<int>> &data) {
        Logger::getInstance().log("FileHandler: Writing summed output to file: " + filename);
        if (data.empty()) {
            Logger::getInstance().log("FileHandler: WARNING - Summed data is empty. Output file " + filename + " will be empty.");
        }

        std::ofstream outfile(filename);
        if (!outfile) {
            ErrorHandler::reportError("FileHandler: Could not open file " + filename + " for writing. Check permissions or directory existence.");
            return false;
        }
        for (const auto &kv : data) {
            long long sum = 0; // Use long long for sum to avoid overflow if counts are large
            for (int count : kv.second) {
                sum += count;
            }
            outfile << "<\"" << kv.first << "\", " << sum << ">\n"; // Original format
        }
        outfile.close();
        Logger::getInstance().log("FileHandler: Successfully wrote summed output for " + std::to_string(data.size()) + " keys to " + filename);
        return true;
    }

    static bool read_mapped_data(const std::string &filename, std::vector<std::pair<std::string, int>> &mapped_data) {
        Logger::getInstance().log("FileHandler: Attempting to read mapped data from file: " + filename);
    
        std::ifstream infile(filename);
        if (!infile) {
            ErrorHandler::reportError("FileHandler: Could not open file " + filename + " for reading.");
            return false; // Return false, don't make it critical here, caller can decide.
        }
    
        std::string line;
        int line_number = 0;
        size_t initial_size = mapped_data.size(); // In case we are appending to existing vector

        while (std::getline(infile, line)) {
            line_number++;
    
            size_t colon = line.find(':');
            if (colon != std::string::npos && colon > 0) { // Ensure colon is not the first char
                try {
                    std::string word = line.substr(0, colon);
                    std::string count_str = line.substr(colon + 1);
    
                    // Trim whitespace
                    word.erase(0, word.find_first_not_of(" \t\r\n"));
                    word.erase(word.find_last_not_of(" \t\r\n") + 1);
                    count_str.erase(0, count_str.find_first_not_of(" \t\r\n"));
                    count_str.erase(count_str.find_last_not_of(" \t\r\n") + 1);
    
                    if (word.empty()) {
                        // Logger::getInstance().log("FileHandler: WARNING - Empty key on line " + std::to_string(line_number) + " in " + filename);
                        continue; 
                    }
                    
                    int count = 0;
                    std::stringstream ss_count(count_str);
                    ss_count >> count;
    
                    if (!ss_count.fail() && ss_count.eof()) { // Ensure entire string was consumed and is valid int
                        mapped_data.emplace_back(word, count);
                    } else {
                         Logger::getInstance().log("FileHandler: WARNING - Invalid count format '" + count_str + "' for key '" + word + "' on line " + std::to_string(line_number) + " in " + filename);
                    }
                } catch (const std::exception &e) {
                    Logger::getInstance().log("FileHandler: ERROR - Exception while processing line " + std::to_string(line_number) + " in " + filename + ": " + line + ". Exception: " + e.what());
                }
            } else {
                 if (!line.empty() && line.find_first_not_of(" \t\r\n") != std::string::npos) { // Report only non-empty, non-whitespace lines
                    Logger::getInstance().log("FileHandler: WARNING - Skipped malformed line (no valid colon separator) " + std::to_string(line_number) + " in " + filename + ": " + line);
                 }
            }
        }
    
        infile.close();
    
        if (mapped_data.size() == initial_size && line_number > 0) { // Read lines but no valid data found
            Logger::getInstance().log("FileHandler: WARNING - No valid key-value data found in file: " + filename + " despite reading " + std::to_string(line_number) + " lines.");
        } else if (mapped_data.size() > initial_size) {
            Logger::getInstance().log("FileHandler: Successfully read " + std::to_string(mapped_data.size() - initial_size) + " new entries from file: " + filename);
        } else if (line_number == 0) {
             Logger::getInstance().log("FileHandler: File " + filename + " was empty or could not be read line by line after opening.");
        }
    
        return true; // True if file was opened, even if no valid data was parsed. Caller checks mapped_data size.
    }

    // Helper to create an empty file, e.g. for _SUCCESS signal
    static bool create_empty_file(const std::string& filepath) {
        Logger::getInstance().log("FileHandler: Creating empty file: " + filepath);
        std::ofstream file(filepath);
        if (!file) {
            ErrorHandler::reportError("FileHandler: Could not create empty file " + filepath, true);
            return false;
        }
        file.close();
        Logger::getInstance().log("FileHandler: Successfully created empty file " + filepath);
        return true;
    }
};
#endif // FILEHANDLER_H (guard typically not needed for #pragma once but good practice)

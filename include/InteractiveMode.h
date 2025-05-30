#ifndef INTERACTIVE_MODE_H
#define INTERACTIVE_MODE_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm> // For std::all_of if used by clean_word indirectly

// Project Headers - Ensure these are accessible from your include paths
#include ".\include\ERROR_Handler.h"
#include ".\include\FileHandler.h"
#include ".\include\Logger.h"
#include ".\include\Mapper_DLL_so.h"
#include ".\include\Reducer_DLL_so.h"

namespace fs = std::filesystem;

// Helper function to check if a string ends with a specific suffix
inline bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Function to encapsulate the interactive mode logic
// Returns 0 on success, non-zero on failure, similar to main's exit codes.
int runInteractiveWorkflow() {
    // Initialize logging
    Logger::getInstance().configureLogFilePath("application.log");
    Logger::getInstance().log("WELCOME TO MAPREDUCE (Interactive Mode)...");

    // Validate input folder
    std::string folder_path;
    std::vector<std::string> input_file_paths_interactive; // Distinct name to avoid conflicts
    std::cout << "Enter the folder path for the directory to be processed: ";
    std::getline(std::cin, folder_path);

    // Dynamically build paths for blank folder and slash type
    std::string blank_folder_path;
    std::string os_slash_type;
    size_t last_slash_pos = folder_path.find_last_of('\\');

    if (last_slash_pos != std::string::npos) {
        blank_folder_path = folder_path.substr(0, last_slash_pos + 1);
        os_slash_type = "\\";
    } else {
        last_slash_pos = folder_path.find_last_of('/');
        if (last_slash_pos != std::string::npos) {
            blank_folder_path = folder_path.substr(0, last_slash_pos + 1);
        } else {
            blank_folder_path = ".";
        }
        os_slash_type = "/";
    }

    // Ensure blank_folder_path ends with a slash if it's just "."
    if (blank_folder_path == "." && !ends_with(blank_folder_path, os_slash_type)) {
        blank_folder_path += os_slash_type;
    }

    if (!FileHandler::validate_directory(folder_path, input_file_paths_interactive, "", false)) {
        Logger::getInstance().log("Invalid input folder path. Exiting.");
        return 1; // Failure
    }

    // Validate output folder
    std::string output_folder_path;
    std::cout << "Enter the folder path for the output directory: ";
    std::getline(std::cin, output_folder_path);
    std::string default_output_path = blank_folder_path + "outputFolder";
    if (!FileHandler::validate_directory(output_folder_path, input_file_paths_interactive, default_output_path, true)) {
        Logger::getInstance().log("Invalid output folder path. Exiting.");
        return 1; // Failure
    }
    if (output_folder_path.empty()) output_folder_path = default_output_path;

    // Validate temporary folder
    std::string temp_folder_path;
    std::cout << "Enter the folder path for the temporary directory for intermediate files: ";
    std::getline(std::cin, temp_folder_path);
    std::string default_temp_path = blank_folder_path + "tempFolder";
    if (!FileHandler::validate_directory(temp_folder_path, input_file_paths_interactive, default_temp_path, true)) {
        Logger::getInstance().log("Invalid temporary folder path. Exiting.");
        return 1; // Failure
    }
    if (temp_folder_path.empty()) temp_folder_path = default_temp_path;

    // Display folder paths and proceed
    std::cout << "Input Folder: " << folder_path << std::endl;
    std::cout << "Output Folder: " << output_folder_path << std::endl;
    std::cout << "Temporary Folder: " << temp_folder_path << std::endl;
    std::cout << "\nAll folder paths validated successfully. Proceeding with MapReduce...\n";

    // Map phase
    std::vector<std::string> extracted_lines;
    for (const auto& file_path_from_interactive_input : input_file_paths_interactive) {
        FileHandler::read_file(file_path_from_interactive_input, extracted_lines);
    }

    std::string mapped_file_path = temp_folder_path + os_slash_type + "mapped_temp.txt";
    Mapper mapper(Logger::getInstance(), ErrorHandler()); // Pass Logger and ErrorHandler objects
    mapper.map_words(extracted_lines, mapped_file_path); // Updated method to map words

    // Reduce phase
    std::vector<std::pair<std::string, int>> mapped_data;
    if (!FileHandler::read_mapped_data(mapped_file_path, mapped_data)) {
        Logger::getInstance().log("ERROR: Failed to read mapped data from " + mapped_file_path + ". Exiting.");
        return 1; // Failure
    }

    if (mapped_data.empty()) {
        Logger::getInstance().log("WARNING: mapped_data is empty. Output file will be empty.");
    }

    std::map<std::string, int> reduced_data;
    ReducerDLLso reducer;
    reducer.reduce(mapped_data, reduced_data);

    if (reduced_data.empty()) {
        Logger::getInstance().log("WARNING: reduced_data is empty. Output file will be empty.");
    }

    // Write results to output file
    std::string output_file_path = output_folder_path + os_slash_type + "output.txt";
    if (!FileHandler::write_output(output_file_path, reduced_data)) {
        Logger::getInstance().log("ERROR: Failed to write output file. Exiting.");
        return 1; // Failure
    }

    // Transform reduced data for summed output
    std::map<std::string, std::vector<int>> transformed_data;
    for (const auto& entry : reduced_data) {
        transformed_data[entry.first] = {entry.second};
    }

    std::string summed_output_path = output_folder_path + os_slash_type + "output_summed.txt";
    if (!FileHandler::write_summed_output(summed_output_path, transformed_data)) {
        Logger::getInstance().log("ERROR: Failed to write summed output file. Exiting.");
        return 1; // Failure
    }

    // Final logs and completion messages
    Logger::getInstance().log("Process complete!");
    Logger::getInstance().log("Mapped data: " + mapped_file_path);
    Logger::getInstance().log("Word counts: " + output_file_path);
    Logger::getInstance().log("Summed counts: " + summed_output_path);

    return 0; // Success
}

#endif // INTERACTIVE_MODE_H
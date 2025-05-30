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
#include "..\include\ERROR_Handler.h"
#include "..\include\FileHandler.h"
#include "..\include\Logger.h"
#include "..\include\Mapper_DLL_so.h"
#include "..\include\Reducer_DLL_so.h"

namespace fs = std::filesystem;

inline bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Function to encapsulate the original interactive mode logic
// Returns 0 on success, non-zero on failure, similar to main's exit codes.
int runInteractiveWorkflow() {
    
    Logger::getInstance().configureLogFilePath("application.log"); 
    Logger::getInstance().setPrefix("[INTERACTIVE] "); 
    Logger::getInstance().log("WELCOME TO MAPREDUCE (Interactive Mode)...");

    std::string folder_path;
    std::vector<std::string> input_file_paths_interactive; // Use a distinct name
    std::cout << "Enter the folder path for the directory to be processed: ";
    std::getline(std::cin, folder_path);

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
            blank_folder_path = "." ; 
        }
        os_slash_type = "/"; 
    }
    // Ensure blank_folder_path ends with a slash if it's just "."
    if (blank_folder_path == "." && !blank_folder_path.ends_with(os_slash_type)) {
         blank_folder_path += os_slash_type;
    }


    // This call uses the original 4-argument validate_directory from your FileHandler.h
    // It populates input_file_paths_interactive
    if (!FileHandler::validate_directory(folder_path, input_file_paths_interactive, "", false)) { // Removed 'const' from input
        Logger::getInstance().log("Invalid input folder path. Exiting.");
        return 1; // Failure
    }

    std::string output_folder_path;
    std::cout << "Enter the folder path for the output directory: ";
    std::getline(std::cin, output_folder_path);
    std::string default_output_path = blank_folder_path + "outputFolder";
    // This call uses the original 4-argument validate_directory
    if (!FileHandler::validate_directory(output_folder_path, input_file_paths_interactive, default_output_path, true)) {
        Logger::getInstance().log("Invalid output folder path. Exiting.");
        return 1; // Failure
    }
    if (output_folder_path.empty()) output_folder_path = default_output_path;


    std::string temp_folder_path;
    std::cout << "Enter the folder path for the temporary directory for intermediate files: ";
    std::getline(std::cin, temp_folder_path);
    std::string default_temp_path = blank_folder_path + "tempFolder";
    // This call uses the original 4-argument validate_directory
    if (!FileHandler::validate_directory(temp_folder_path, input_file_paths_interactive, default_temp_path, true)) {
        Logger::getInstance().log("Invalid temporary folder path. Exiting.");
        return 1; // Failure
    }
    if (temp_folder_path.empty()) temp_folder_path = default_temp_path;


    std::cout << "Input Folder: " << folder_path << std::endl;
    std::cout << "Output Folder: " << output_folder_path << std::endl;
    std::cout << "Temporary Folder: " << temp_folder_path << std::endl;
    std::cout << "\nAll folder paths validated successfully. Proceeding with MapReduce...\n";

    std::vector<std::string> extracted_lines;
    // input_file_paths_interactive should be populated by your original FileHandler::validate_directory
    for (const auto &file_path_from_interactive_input : input_file_paths_interactive) { 
        FileHandler::read_file(file_path_from_interactive_input, extracted_lines);
    }

    // MAP PHASE (Interactive - original logic)
    // This uses the 2-argument map_words from your Mapper_DLL_so.h
    std::string mapped_file_path = temp_folder_path + os_slash_type + "mapped_temp.txt";
    MapperDLLso mapper; // Declare mapper object
    mapper.map_words(extracted_lines, mapped_file_path);
    Logger::getInstance().log("Interactive Mode: Map phase produced intermediate file: " + mapped_file_path);


    // REDUCE PHASE (Interactive - original logic)
    std::vector<std::pair<std::string, int>> mapped_data;
    if (!FileHandler::read_mapped_data(mapped_file_path, mapped_data)) {
        Logger::getInstance().log("ERROR: Failed to read mapped data from " + mapped_file_path + ". Exiting.\n");
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

    std::string output_file_path = output_folder_path + os_slash_type + "output.txt";
    if (!FileHandler::write_output(output_file_path, reduced_data)) {
        Logger::getInstance().log("ERROR: Failed to write output file. Exiting.\n");
        return 1; // Failure
    }

    std::map<std::string, std::vector<int>> transformed_data;
    for (const auto &entry : reduced_data) {
        transformed_data[entry.first] = {entry.second};
    }

    std::string summed_output_path = output_folder_path + os_slash_type + "output_summed.txt"; 
    if (!FileHandler::write_summed_output(summed_output_path, transformed_data)) {
        Logger::getInstance().log("ERROR: Failed to write summed output file. Exiting.\n");
        return 1; // Failure
    }

    Logger::getInstance().log("Interactive Mode: Process complete!");
    Logger::getInstance().log("Mapped data: " + mapped_file_path); 
    Logger::getInstance().log("Word counts: " + output_file_path);
    Logger::getInstance().log("Summed counts: " + summed_output_path);

    return 0; // Success
}

#endif // INTERACTIVE_MODE_H
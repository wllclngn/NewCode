#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include "ERROR_Handler.h"
#include "FileHandler.h"
#include "Logger.h"
#include "Mapper.h"
#include "Reducer.h"

namespace fs = std::filesystem;

int main()
{ 
    // Initialize logging
    Logger::getInstance().configureLogFilePath("application.log");
    Logger::getInstance().log("WELCOME TO MAPREDUCE...");

    // Validate Input folder
    std::string folder_path;
    std::vector<std::string> input_file_paths;
    std::cout << "Enter the folder path for the directory to be processed: ";
    std::getline(std::cin, folder_path);

    // Build out folder_path from user input for blank, "", user input directories.
    std::string blank_folder_path;
    std::string os_slash_type;
    size_t last_slash_pos = folder_path.find_last_of('\\');

    // Check if a backslash was found
    if (last_slash_pos != std::string::npos) {
        // Dynamically build out folder_path for blank outPutFolder and tempFolder
        blank_folder_path = folder_path.substr(0, last_slash_pos+1);
        os_slash_type = "\\";
    } else {
        last_slash_pos = folder_path.find_last_of('/');
        // Dynamically build out folder_path for blank outPutFolder and tempFolder
        blank_folder_path = folder_path.substr(0, last_slash_pos+1);
        os_slash_type = "/";
    }

    // Validate Output folder
    std::string output_folder_path;
    std::cout << "Enter the folder path for the output directory: ";
    std::getline(std::cin, output_folder_path);

    if (!FileHandler::validate_directory(output_folder_path, input_file_paths, blank_folder_path + "outputFolder", true)) {
        Logger::getInstance().log("Invalid output folder path. Exiting.");
        return 1;
    }

    // Validate Temporary folder
    std::string temp_folder_path;
    std::cout << "Enter the folder path for the temporary directory for intermediate files: ";
    std::getline(std::cin, temp_folder_path);
    
    if (!FileHandler::validate_directory(temp_folder_path, input_file_paths, blank_folder_path + "tempFolder", true)) {
        Logger::getInstance().log("Invalid temporary folder path. Exiting.");
        return 1;
    }

    // Display the validated folder paths
    std::cout << "Input Folder: " << folder_path << std::endl;
    std::cout << "Output Folder: " << output_folder_path << std::endl;
    std::cout << "Temporary Folder: " << temp_folder_path << std::endl;

    // Proceed with the rest of the program
    std::cout << "\nAll folder paths validated successfully. Proceeding with MapReduce...\n";

    // Map phase
    std::vector<std::string> extracted_lines;
    for (const auto &file_path : input_file_paths) {
        FileHandler::read_file(file_path, extracted_lines);
    }

    std::string mapped_file_path = temp_folder_path + os_slash_type + "mapped_temp.txt";

    Mapper mapper;
    mapper.map_words(extracted_lines, mapped_file_path);

    // Reduce phase
    std::vector<std::pair<std::string, int>> mapped_data;


    if (!FileHandler::read_mapped_data(mapped_file_path, mapped_data)) {
        Logger::getInstance().log("ERROR: Failed to read mapped data. Exiting.\n");
        return 1;
    }

    if (mapped_data.empty()) {
        Logger::getInstance().log("WARNING: mapped_data is empty. Output file will be empty.");
    }

    std::map<std::string, int> reduced_data;
    Reducer reducer;
    reducer.reduce(mapped_data, reduced_data);

    if (reduced_data.empty()) {
        Logger::getInstance().log("WARNING: reduced_data is empty. Output file will be empty.");
    }

    // Write outputs
    std::string output_file_path = output_folder_path+ os_slash_type + "output.txt";
    if (!FileHandler::write_output(output_file_path, reduced_data)) {
        Logger::getInstance().log("ERROR: Failed to write output file. Exiting.\n");
        return 1;
    }

    // Transform reducedData to match the expected format of write_summed_output
    std::map<std::string, std::vector<int>> transformed_data;
    for (const auto &entry : reduced_data) {
        transformed_data[entry.first] = {entry.second};
    }

    std::string summed_output_path = output_folder_path+ os_slash_type + "/output_summed.txt";
    if (!FileHandler::write_summed_output(summed_output_path, transformed_data)) {
        Logger::getInstance().log("ERROR: Failed to write summed output file. Exiting.\n");
        return 1;
    }

    // Display results
    Logger::getInstance().log("Process complete!");
    Logger::getInstance().log("Mapped data: mapped_temp.txt");
    Logger::getInstance().log("Word counts: output.txt");
    Logger::getInstance().log("Summed counts: output_summed.txt");

    return 0;
}
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

    if (!FileHandler::validate_directory(folder_path, input_file_paths)) {
        Logger::getInstance().log("Invalid input folder path. Exiting.");
        return 1;
    }
    if (!folder_path.empty() && (folder_path.back() == '/' || folder_path.back() == '\\'))
        folder_path.pop_back();

    // Validate Output folder
    std::string output_folder_path;
    std::vector<std::string> output_file_paths;
    std::cout << "Enter the folder path for the output directory: ";
    std::getline(std::cin, output_folder_path);

    if (!FileHandler::validate_directory(output_folder_path, output_file_paths)) {
        Logger::getInstance().log("Invalid output folder path. Exiting.");
        return 1;
    }
    if (!output_folder_path.empty() && (output_folder_path.back() == '/' || output_folder_path.back() == '\\'))
        output_folder_path.pop_back();

    // Ensure the output directory exists
    if (!fs::exists(output_folder_path)) {
        Logger::getInstance().log("Output directory does not exist. Creating it...");
        fs::create_directories(output_folder_path);
    }

    // Validate Temporary folder
    std::string temp_folder_path;
    std::vector<std::string> temp_file_paths;
    std::cout << "Enter the folder path for the temporary directory for intermediate files: ";
    std::getline(std::cin, temp_folder_path);
    
    if (!FileHandler::validate_directory(temp_folder_path, temp_file_paths)) {
        Logger::getInstance().log("Invalid temporary folder path. Exiting.");
        return 1;
    }
    if (!temp_folder_path.empty() && (temp_folder_path.back() == '/' || temp_folder_path.back() == '\\'))
        temp_folder_path.pop_back();

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

    Mapper mapper;
    mapper.map_words(extracted_lines, temp_folder_path);

    // Reduce phase
    std::vector<std::pair<std::string, int>> mapped_data;
    std::string mapped_file_path = temp_folder_path + "/mapped_temp.txt";


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
    std::string output_file_path = output_folder_path + "/output.txt";
    if (!FileHandler::write_output(output_file_path, reduced_data)) {
        Logger::getInstance().log("ERROR: Failed to write output file. Exiting.\n");
        return 1;
    }

    // Transform reducedData to match the expected format of write_summed_output
    std::map<std::string, std::vector<int>> transformed_data;
    for (const auto &entry : reduced_data) {
        transformed_data[entry.first] = {entry.second};
    }

    std::string summed_output_path = output_folder_path + "/output_summed.txt";
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
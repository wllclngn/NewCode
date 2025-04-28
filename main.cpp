#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include "fileHandler.h"
#include "mapper.h"
#include "reducer.h"

namespace fs = boost::filesystem;

int main()
{
    // Welcome message
    std::cerr << "WELCOME TO MAPREDUCE..." << std::endl;

    // Validate Input folder
    std::string folder_path;
    std::cout << "Enter the folder path for the directory to be processed: ";
    std::getline(std::cin, folder_path);

    std::string folder_path = validateFolderPath("Input");
    if (!output_folder_path.empty() && (output_folder_path.back() == '/' || output_folder_path.back() == '\\'))
        output_folder_path.pop_back();

    // Validate Output folder    
    std::string output_folder_path;
    std::cout << "Enter the folder path for the output directory: ";
    std::getline(std::cin, output_folder_path);

    std::string output_folder_path = validateFolderPath("Output");
    if (!output_folder_path.empty() && (output_folder_path.back() == '/' || output_folder_path.back() == '\\'))
        output_folder_path.pop_back();

    // Validate Temporary folder    std::string temp_folder_path;
    std::cout << "Enter the folder path for the temporary directory for intermediate files: ";
    std::getline(std::cin, temp_folder_path);
    std::string temp_folder_path = validateFolderPath("Temporary");
    if (!output_folder_path.empty() && (output_folder_path.back() == '/' || output_folder_path.back() == '\\'))
        output_folder_path.pop_back();

    // Display the validated folder paths
    std::cout << "Input Folder: " << folder_path << std::endl;
    std::cout << "Output Folder: " << output_folder_path << std::endl;
    std::cout << "Temporary Folder: " << temp_folder_path << std::endl;

    // Proceed with the rest of the program
    std::cout << "\nAll folder paths validated successfully. Proceeding with MapReduce...\n";

    // Check if the string contains '/' or '\'
    std::string SysPathSlash;

    if (temp_folder_path.find('/') != std::string::npos) {
        SysPathSlash = "/"; // Assign "/" to SysPathSlash
        std::cout << "The string contains '/'. Assigned '/' to SysPathSlash." << std::endl;
    } else if (temp_folder_path.find('\\') != std::string::npos) {
        SysPathSlash = "\\"; // Assign "\" to SysPathSlash
        std::cout << "The string contains '\\'. Assigned '\\' to SysPathSlash." << std::endl;
    }

    // Prepare file paths
    std::string file_list_path = temp_folder_path + SysPathSlash + "fileNames.txt";

    // Handle input directory and create temp files
    if (!FileHandler::create_temp_log_file(folder_path, file_list_path) ||
        !FileHandler::write_filenames_to_file(folder_path, file_list_path))
    {
        std::cerr << "ERROR: Failed to prepare temp files. Exiting." << std::endl;
        return 1;
    }

    // Read file names from the temporary file
    std::vector<std::string> file_names;
    if (!FileHandler::read_file(file_list_path, file_names))
    {
        std::cerr << "ERROR: Failed to read fileNames.txt. Exiting." << std::endl;
        return 1;
    }

    // Map phase
    std::vector<std::string> extracted_lines;
    std::string temp_input_path = temp_folder_path + "/tempInput.txt";
    if (!FileHandler::extract_values_from_temp_input(extracted_lines, temp_input_path))
    {
        std::cerr << "ERROR: Failed to extract lines from tempInput.txt. Exiting." << std::endl;
        return 1;
    }

    Mapper mapper;
    mapper.map_words(extracted_lines, temp_folder_path);

    // Reduce phase
    std::vector<std::pair<std::string, int>> mapped_data;
    std::string mapped_file_path = temp_folder_path + "/mapped_temp.txt";
    if (!FileHandler::read_mapped_data(mapped_file_path, mapped_data))
    {
        std::cerr << "ERROR: Failed to read mapped data. Exiting." << std::endl;
        return 1;
    }

    Reducer reducer;
    reducer.reduce(mapped_data);

    // Write outputs
    std::string output_file_path = output_folder_path + "/output.txt";
    if (!FileHandler::write_output(output_file_path, reducer.get_reduced_data()))
    {
        std::cerr << "ERROR: Failed to write output file. Exiting." << std::endl;
        return 1;
    }

    std::string summed_output_path = output_folder_path + "/output_summed.txt";
    if (!FileHandler::write_summed_output(summed_output_path, reducer.get_reduced_data()))
    {
        std::cerr << "ERROR: Failed to write summed output file. Exiting." << std::endl;
        return 1;
    }

    // Display results
    std::cout << "\n Process complete!" << std::endl;
    std::cout << "   Mapped data: mapped_temp.txt" << std::endl;
    std::cout << "   Word counts: output.txt" << std::endl;
    std::cout << "   Summed counts: output_summed.txt" << std::endl;

    return 0;
}

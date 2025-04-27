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


// Main function
int main()
{
    std::cerr << "WELCOME TO MAPREDUCE..." << std::endl;

    std::string inputPath, outputPath, tempPath;

    // Validate Input folder
    if (!FolderPathValidator::user_input_directory("Input", inputPath)) {
        std::cerr << "Input folder validation aborted by user." << std::endl;
        return 1; // Exit the program gracefully
    }

    // Validate Output folder
    if (!FolderPathValidator::user_input_directory("Output", outputPath)) {
        std::cerr << "Output folder validation aborted by user." << std::endl;
        return 1; // Exit the program gracefully
    }

    // Validate Temporary folder
    if (!FolderPathValidator::user_input_directory("Temporary", tempPath)) {
        std::cerr << "Temporary folder validation aborted by user." << std::endl;
        return 1; // Exit the program gracefully
    }

    // Display the validated folder paths
    std::cout << "Input Folder: " << inputPath << std::endl;
    std::cout << "Output Folder: " << outputPath << std::endl;
    std::cout << "Temporary Folder: " << tempPath << std::endl;

    // Proceed with the rest of the program
    std::cout << "\nAll folder paths validated successfully. Proceeding with MapReduce...\n";

    std::string file_list_path = tempPath + "/fileNames.txt";

    // Handle input directory and create temp files
    if (!FileHandler::create_temp_log_file(folder_path, file_list_path) ||
        !FileHandler::write_filenames_to_file(folder_path, file_list_path))
    {
        std::cerr << "❌ Failed to prepare temp files. Exiting." << std::endl;
        return 1;
    }

    // Read file names
    std::vector<std::string> file_names;
    if (!FileHandler::read_file(file_list_path, file_names))
    {
        std::cerr << "❌ Failed to read fileNames.txt. Exiting." << std::endl;
        return 1;
    }

    // Map phase
    std::vector<std::string> extracted_lines;
    std::string temp_input_path = temp_folder_path + "/tempInput.txt";
    if (!FileHandler::extract_values_from_temp_input(extracted_lines, temp_input_path))
    {
        std::cerr << "❌ Failed to extract lines from tempInput.txt. Exiting." << std::endl;
        return 1;
    }

    Mapper mapper;
    mapper.map_words(extracted_lines, temp_folder_path);

    // Reduce phase
    std::vector<std::pair<std::string, int>> mapped_data;
    std::string mapped_file_path = temp_folder_path + "/mapped_temp.txt";
    if (!FileHandler::read_mapped_data(mapped_file_path, mapped_data))
    {
        std::cerr << "❌ Failed to read mapped data. Exiting." << std::endl;
        return 1;
    }

    Reducer reducer;
    reducer.reduce(mapped_data);

    // Write outputs
    std::string output_file_path = output_folder_path + "/output.txt";
    if (!FileHandler::write_output(output_file_path, reducer.get_reduced_data()))
    {
        std::cerr << "❌ Failed to write output file. Exiting." << std::endl;
        return 1;
    }

    std::string summed_output_path = output_folder_path + "/output_summed.txt";
    if (!FileHandler::write_summed_output(summed_output_path, reducer.get_reduced_data()))
    {
        std::cerr << "❌ Failed to write summed output file. Exiting." << std::endl;
        return 1;
    }

    std::cout << "\n✅ Process complete!" << std::endl;
    std::cout << "  - Mapped data: mapped_temp.txt" << std::endl;
    std::cout << "  - Word counts: output.txt" << std::endl;
    std::cout << "  - Summed counts: output_summed.txt" << std::endl;

    return 0;
}
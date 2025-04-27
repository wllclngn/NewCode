#include <iostream>
#include <vector>
#include <string>
#include "fileHandler.h"
#include "mapper.h"
#include "reducer.h"

int main()
{
    std::cerr << "WELCOME TO MAPREDUCE..." << std::endl;

    std::string inputFolder, outputFolder, tempFolder;

    // Get user input for folder paths
    std::cout << "Enter the path for the Input folder: ";
    std::getline(std::cin, inputFolder);

    std::cout << "Enter the path for the Output folder: ";
    std::getline(std::cin, outputFolder);

    std::cout << "Enter the path for the Temporary folder: ";
    std::getline(std::cin, tempFolder);

    // Log the entered paths
    std::cout << "\nInput Folder: " << inputFolder << std::endl;
    std::cout << "Output Folder: " << outputFolder << std::endl;
    std::cout << "Temporary Folder: " << tempFolder << std::endl;

    // Validate folder paths
    if (!FileHandler::validate_directory(inputFolder) ||
        !FileHandler::validate_directory(outputFolder) ||
        !FileHandler::validate_directory(tempFolder))
    {
        std::cerr << "❌ Folder validation failed. Exiting." << std::endl;
        return 1;
    }

    // Log validated folder paths
    std::cout << "Input Folder: " << inputFolder << std::endl;
    std::cout << "Output Folder: " << outputFolder << std::endl;
    std::cout << "Temporary Folder: " << tempFolder << std::endl;

    // Prepare file list
    std::string fileListPath = tempFolder + "/fileNames.txt";
    if (!FileHandler::write_filenames_to_file(inputFolder, fileListPath))
    {
        std::cerr << "❌ Failed to create file list. Exiting." << std::endl;
        return 1;
    }

    // Read file names into a vector
    std::vector<std::string> fileNames;
    if (!FileHandler::read_file(fileListPath, fileNames))
    {
        std::cerr << "❌ Failed to read file list. Exiting." << std::endl;
        return 1;
    }

    // Map phase
    Mapper mapper;
    std::vector<std::string> extractedLines;
    for (const auto &fileName : fileNames)
    {
        std::vector<std::string> lines;
        if (FileHandler::read_file(inputFolder + "/" + fileName, lines))
        {
            extractedLines.insert(extractedLines.end(), lines.begin(), lines.end());
        }
    }

    std::string mappedFilePath = tempFolder + "/mapped_temp.txt";
    mapper.map_words(extractedLines, mappedFilePath);

    // Reduce phase
    std::vector<std::pair<std::string, int>> mappedData;
    if (!FileHandler::read_mapped_data(mappedFilePath, mappedData))
    {
        std::cerr << "❌ Failed to read mapped data. Exiting." << std::endl;
        return 1;
    }

    Reducer reducer;
    reducer.reduce(mappedData);

    // Write final outputs
    std::string outputFilePath = outputFolder + "/output.txt";
    if (!FileHandler::write_output(outputFilePath, reducer.get_reduced_data()))
    {
        std::cerr << "❌ Failed to write output file. Exiting." << std::endl;
        return 1;
    }

    std::cout << "\n✅ Process complete!" << std::endl;
    std::cout << "  - File List: " << fileListPath << std::endl;
    std::cout << "  - Mapped Data: " << mappedFilePath << std::endl;
    std::cout << "  - Final Output: " << outputFilePath << std::endl;

    return 0;
}
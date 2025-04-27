#include "FolderPathValidator.cpp" // Include the function

int main()
{
    std::cerr << "WELCOME TO MAPREDUCE..." << std::endl;

    // Validate Input folder
    std::string folder_path = validateFolderPath("Input");

    // Validate Output folder
    std::string output_folder_path = validateFolderPath("Output");

    // Validate Temporary folder
    std::string temp_folder_path = validateFolderPath("Temporary");

    std::cout << "Input Folder: " << folder_path << std::endl;
    std::cout << "Output Folder: " << output_folder_path << std::endl;
    std::cout << "Temporary Folder: " << temp_folder_path << std::endl;

    return 0;
}
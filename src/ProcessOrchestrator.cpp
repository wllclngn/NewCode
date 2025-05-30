#include "..\include\ProcessOrchestrator.h"
#include "..\include\Logger.h"
#include "..\include\ERROR_Handler.h"
#include "..\include\ThreadPool.h"

#include <fstream>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Add namespaces if required
namespace fs = std::filesystem;

// Implementation of the ProcessOrchestratorDLL class

void ProcessOrchestratorDLL::runFinalReducer(const std::string& outputDir, const std::string& tempDir) {
    Logger::getInstance().log("Starting final reduction...");

    // Example logic for final reduction
    std::map<std::string, int> finalResults;

    // Read intermediate reducer outputs
    for (const auto& entry : fs::directory_iterator(tempDir)) {
        if (entry.path().extension() == ".txt") {
            std::ifstream inFile(entry.path());
            std::string key;
            int value;

            while (inFile >> key >> value) {
                finalResults[key] += value;
            }
        }
    }

    // Write final results to output directory
    std::ofstream outFile(outputDir + "/final_results.txt");
    if (!outFile.is_open()) {
        Logger::getInstance().log("ERROR: Could not open final_results.txt for writing.", Logger::Level::ERROR);
        return;
    }

    for (const auto& [key, value] : finalResults) {
        outFile << key << "\t" << value << "\n";
    }

    Logger::getInstance().log("Final reduction completed.");
}

// Placeholder for other ProcessOrchestratorDLL methods
// Add additional methods or logic as needed for this class.
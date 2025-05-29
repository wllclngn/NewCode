#ifndef PROCESS_ORCHESTRATOR_DLL_H
#define PROCESS_ORCHESTRATOR_DLL_H

#include <string>
#include <vector>
#include <map>
#include <iostream> // For std::system errors, though ErrorHandler should be preferred
#include <cstdlib>  // For std::system
#include <sstream>  // For ostringstream
#include <algorithm>// For std::sort
#include <filesystem> // For fs::exists, fs::file_size etc.

// Project Headers
#include "FileHandler.h"   // Used by implementations
#include "Logger.h"        // Used by implementations
#include "ErrorHandler.h"  // Used by implementations
#include "Mapper_DLL_so.h" // Used by runMapper
#include "Reducer_DLL_so.h"// Used by runReducer

// Export macro for cross-platform compatibility
#if defined(_WIN32) || defined(_WIN64)
#define DLL_so_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define DLL_so_EXPORT __attribute__((visibility("default")))
#else
#define DLL_so_EXPORT
#endif

namespace fs = std::filesystem;

class DLL_so_EXPORT ProcessOrchestratorDLL {
public:
    virtual ~ProcessOrchestratorDLL() {}

    // --- Process Management Utility ---
    // A basic, synchronous process launcher.
    // Returns the exit code from std::system, or a non-zero value on immediate launch failure.
    virtual int launchProcess(const std::string& command) {
        Logger::getInstance().log("ORCHESTRATOR_DLL: Executing command: " + command);
        int result = std::system(command.c_str()); // std::system is synchronous
        if (result != 0) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL: Command returned non-zero (" + std::to_string(result) + "): " + command);
        }
        return result; 
    }

    // --- Core Operational Logic Functions ---
    virtual bool runController(const std::string& executablePath,
                               const std::string& inputDir,
                               const std::string& outputDir,
                               const std::string& tempDir,
                               int numMappers,
                               int numReducers) {
        Logger::getInstance().log("ORCHESTRATOR_DLL: Controller starting. M=" + std::to_string(numMappers) + ", R=" + std::to_string(numReducers));

        if (numMappers <= 0 || numReducers <= 0) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Number of mappers and reducers must be positive.", true);
            return false;
        }

        if (!setupDirectories_impl(inputDir, outputDir, tempDir)) {
            return false;
        }

        std::vector<std::vector<std::string>> mapperFileAssignments;
        if (!distributeInputFiles_impl(inputDir, numMappers, mapperFileAssignments)) {
            return false; // Error already logged
        }
        
        bool noInputFiles = true;
        for(const auto& files : mapperFileAssignments) {
            if (!files.empty()) {
                noInputFiles = false;
                break;
            }
        }

        if (noInputFiles) {
             Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: No input files to process after distribution.");
             if (!FileHandler::create_empty_file(outputDir + "/_SUCCESS")) return false;
             Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: MapReduce job completed successfully (no input files).");
             return true;
        }

        if (!launchMapperProcesses_impl(executablePath, tempDir, numMappers, numReducers, "DEFAULT_MAPPER_LOG_", mapperFileAssignments)) {
            return false;
        }
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: All mapper processes launched (assumed complete due to synchronous std::system).");

        if (!launchReducerProcesses_impl(executablePath, outputDir, tempDir, numReducers, "DEFAULT_REDUCER_LOG_")) {
            return false;
        }
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: All reducer processes launched (assumed complete due to synchronous std::system).");

        if (!performFinalAggregation_impl(outputDir, numReducers, "final_result.txt")) {
            return false;
        }

        if (!FileHandler::create_empty_file(outputDir + "/_SUCCESS")) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to create _SUCCESS file.", true);
            return false;
        }

        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: MapReduce job completed successfully.");
        return true;
    }

    virtual bool runMapper(const std::string& tempDir,
                           int mapperId,
                           int numReducers,
                           const std::vector<std::string>& inputFilePaths) {
        Logger::getInstance().log("ORCHESTRATOR_DLL: Mapper " + std::to_string(mapperId) + " starting for " + std::to_string(inputFilePaths.size()) + " files.");

        MapperDLLso mapperInstance; // Assuming Mapper_DLL_so.h provides this class
        std::vector<std::string> allLines;

        for (const auto& filePath : inputFilePaths) {
            Logger::getInstance().log("ORCHESTRATOR_DLL_MAPPER " + std::to_string(mapperId) + ": Processing file " + filePath);
            std::vector<std::string> linesForFile;
            if (!FileHandler::read_file(filePath, linesForFile)) {
                ErrorHandler::reportError("ORCHESTRATOR_DLL_MAPPER_" + std::to_string(mapperId) + ": Failed to read input file " + filePath, true);
                return false;
            }
            allLines.insert(allLines.end(), linesForFile.begin(), linesForFile.end());
        }

        if (allLines.empty() && !inputFilePaths.empty()) {
            Logger::getInstance().log("ORCHESTRATOR_DLL_MAPPER " + std::to_string(mapperId) + ": All assigned input files were empty.");
        } else if (inputFilePaths.empty()) {
             Logger::getInstance().log("ORCHESTRATOR_DLL_MAPPER " + std::to_string(mapperId) + ": No input files assigned.");
             return true; // No work is a success for the mapper itself
        }
        
        Logger::getInstance().log("ORCHESTRATOR_DLL_MAPPER " + std::to_string(mapperId) + ": Executing map_words for " + std::to_string(allLines.size()) + " total lines.");
        try {
            mapperInstance.map_words(allLines, tempDir, mapperId, numReducers);
        } catch (const std::exception& e) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_MAPPER_" + std::to_string(mapperId) + ": Exception during map_words: " + std::string(e.what()), true);
            return false;
        }

        Logger::getInstance().log("ORCHESTRATOR_DLL_MAPPER " + std::to_string(mapperId) + " finished successfully.");
        return true;
    }

    virtual bool runReducer(const std::string& outputDir,
                            const std::string& tempDir,
                            int reducerId) {
        Logger::getInstance().log("ORCHESTRATOR_DLL: Reducer " + std::to_string(reducerId) + " starting.");

        ReducerDLLso reducerInstance; // Assuming Reducer_DLL_so.h provides this
        std::vector<std::pair<std::string, int>> allMappedDataForPartition;

        std::vector<std::string> allTempFiles;
        if (!FileHandler::get_files_in_directory(tempDir, allTempFiles)) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_REDUCER_" + std::to_string(reducerId) + ": Could not list files in temp directory " + tempDir, true);
            return false;
        }

        std::string partitionFileSuffix = "_partition" + std::to_string(reducerId) + ".tmp";
        for (const auto& tempFilePathStr : allTempFiles) {
            fs::path tempFileFsPath(tempFilePathStr); // Use fs::path for filename operations
            std::string filename = tempFileFsPath.filename().string();

            if (filename.rfind("mapper", 0) == 0 &&
                filename.length() > partitionFileSuffix.length() &&
                filename.substr(filename.length() - partitionFileSuffix.length()) == partitionFileSuffix)
            {
                Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + ": Reading intermediate file " + tempFilePathStr);
                if (!FileHandler::read_mapped_data(tempFilePathStr, allMappedDataForPartition)) {
                    ErrorHandler::reportError("ORCHESTRATOR_DLL_REDUCER_" + std::to_string(reducerId) + ": Failed to read or parse intermediate file " + tempFilePathStr + ". This may lead to incomplete results.", false);
                }
            }
        }
        
        if (allMappedDataForPartition.empty()) {
            Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + ": No data found for this partition after reading intermediate files.");
            std::map<std::string, int> emptyData;
            std::string outputFilePath = outputDir + "/result_partition" + std::to_string(reducerId) + ".txt";
            if (!FileHandler::write_output(outputFilePath, emptyData)) {
                 ErrorHandler::reportError("ORCHESTRATOR_DLL_REDUCER_" + std::to_string(reducerId) + ": Failed to write empty output file " + outputFilePath, true);
                 return false;
            }
            Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + " finished successfully (no data).");
            return true;
        }

        Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + ": Sorting " + std::to_string(allMappedDataForPartition.size()) + " key-value pairs.");
        std::sort(allMappedDataForPartition.begin(), allMappedDataForPartition.end(), 
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                      return a.first < b.first;
                  });

        std::map<std::string, int> reducedData;
        Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + ": Executing reduce operation.");
        try {
            reducerInstance.reduce(allMappedDataForPartition, reducedData);
        } catch (const std::exception& e) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_REDUCER_" + std::to_string(reducerId) + ": Exception during reduce: " + std::string(e.what()), true);
            return false;
        }

        std::string outputFilePath = outputDir + "/result_partition" + std::to_string(reducerId) + ".txt";
        Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + ": Writing final output to " + outputFilePath);
        if (!FileHandler::write_output(outputFilePath, reducedData)) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_REDUCER_" + std::to_string(reducerId) + ": Failed to write final output file " + outputFilePath, true);
            return false;
        }

        Logger::getInstance().log("ORCHESTRATOR_DLL_REDUCER " + std::to_string(reducerId) + " finished successfully.");
        return true;
    }

private:
    // --- Helper Methods for Controller Logic ---
    // Made these private members to access `this->launchProcess()` if needed,
    // or they can call the public virtual `launchProcess` directly.

    std::string buildCommandString_impl(
        const std::string& executablePath, 
        const std::string& mode, 
        const std::vector<std::string>& args,
        const std::vector<std::string>& fileArgs = {}) const
    {
        std::ostringstream cmd;
        cmd << "\"" << executablePath << "\" " << mode; // Basic quoting for executable path
        for (const auto& arg : args) {
            cmd << " \"" << arg << "\""; // Quote individual arguments
        }
        for (const auto& fileArg : fileArgs) {
            cmd << " \"" << fileArg << "\""; // Quote file arguments
        }
        return cmd.str();
    }

    bool setupDirectories_impl(const std::string& inputDir, const std::string& outputDir, const std::string& tempDir) const {
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Setting up directories...");
        if (!FileHandler::validate_directory(inputDir, false)) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Input directory " + inputDir + " is not valid or does not exist.", true);
            return false;
        }
        if (!FileHandler::validate_directory(outputDir, true)) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to validate/create output directory " + outputDir, true);
            return false;
        }
        if (!FileHandler::validate_directory(tempDir, true)) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to validate/create temporary directory " + tempDir, true);
            return false;
        }
        return true;
    }

    bool distributeInputFiles_impl(const std::string& inputDir, int numMappers, std::vector<std::vector<std::string>>& mapperFileAssignments) const {
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Distributing input files from " + inputDir);
        std::vector<std::string> allInputFiles;
        if (!FileHandler::get_files_in_directory(inputDir, allInputFiles, ".txt")) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to get input files from " + inputDir);
            return false;
        }

        if (allInputFiles.empty()) {
            Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: No input files found in " + inputDir + ". Nothing to map.");
            return true; 
        }

        mapperFileAssignments.assign(numMappers, std::vector<std::string>());
        for (size_t i = 0; i < allInputFiles.size(); ++i) {
            mapperFileAssignments[i % numMappers].push_back(allInputFiles[i]);
        }
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Distributed " + std::to_string(allInputFiles.size()) + " files among " + std::to_string(numMappers) + " mappers.");
        return true;
    }

    bool launchMapperProcesses_impl(
        const std::string& executablePath, 
        const std::string& tempDir, 
        int numMappers, 
        int numReducers,
        const std::string& defaultLogPathPrefix, // e.g., "DEFAULT_MAPPER_LOG_"
        const std::vector<std::vector<std::string>>& mapperFileAssignments)
    {
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Launching " + std::to_string(numMappers) + " mapper processes...");
        for (int i = 0; i < numMappers; ++i) {
            if (mapperFileAssignments[i].empty()) {
                Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Mapper " + std::to_string(i) + " has no files assigned, skipping launch.");
                continue;
            }
            std::vector<std::string> args;
            args.push_back(tempDir);
            args.push_back(std::to_string(i)); // mapperId
            args.push_back(std::to_string(numReducers));
            // Construct log path: if prefix is "DEFAULT", it signals main.cpp to use its default logic
            // Otherwise, main.cpp should construct it as tempDir + prefix + id + .log
            // For now, ProcessOrchestratorDLL will pass "DEFAULT" or a fully constructed path
            // Let's stick to "DEFAULT" and let main.cpp decide the actual path.
            args.push_back("DEFAULT"); // Log file path placeholder for main.cpp to interpret

            std::string cmd = buildCommandString_impl(executablePath, "mapper", args, mapperFileAssignments[i]);
            
            if (this->launchProcess(cmd) != 0) { // Call the virtual member launchProcess
                ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to launch or error in mapper " + std::to_string(i) + ". Command: " + cmd, true);
                return false;
            }
        }
        return true;
    }
    
    bool launchReducerProcesses_impl(
        const std::string& executablePath,
        const std::string& outputDir,
        const std::string& tempDir,
        int numReducers,
        const std::string& defaultLogPathPrefix) // e.g., "DEFAULT_REDUCER_LOG_"
    {
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Launching " + std::to_string(numReducers) + " reducer processes...");
        for (int i = 0; i < numReducers; ++i) {
            std::vector<std::string> args;
            args.push_back(outputDir);
            args.push_back(tempDir);
            args.push_back(std::to_string(i)); // reducerId
            args.push_back("DEFAULT"); // Log file path placeholder

            std::string cmd = buildCommandString_impl(executablePath, "reducer", args);
            if (this->launchProcess(cmd) != 0) { // Call the virtual member launchProcess
                ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to launch or error in reducer " + std::to_string(i) + ". Command: " + cmd, true);
                return false;
            }
        }
        return true;
    }

    bool performFinalAggregation_impl(const std::string& outputDir, int numReducers, const std::string& finalOutputFilename) const {
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Performing final aggregation of reducer outputs...");
        std::map<std::string, int> finalAggregatedData;
        bool anyReducerOutputFound = false;

        for (int i = 0; i < numReducers; ++i) {
            std::string reducerOutputFile = outputDir + "/result_partition" + std::to_string(i) + ".txt";
            if (!fs::exists(reducerOutputFile)) {
                Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Reducer output file " + reducerOutputFile + " not found. Skipping.");
                continue; 
            }
            anyReducerOutputFound = true;
            std::vector<std::pair<std::string, int>> reducerPartitionData;
            if (FileHandler::read_mapped_data(reducerOutputFile, reducerPartitionData)) {
                for (const auto& pairVal : reducerPartitionData) { // Renamed pair to pairVal to avoid conflict
                    finalAggregatedData[pairVal.first] += pairVal.second;
                }
            } else {
                ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Issues reading reducer output file: " + reducerOutputFile + ". Aggregation may be incomplete.", false); 
            }
        }
        
        if (!anyReducerOutputFound && numReducers > 0) {
            Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: No reducer output files found for aggregation. Final result will be empty.");
        }

        std::string finalOutputPath = outputDir + "/" + finalOutputFilename;
        if (!FileHandler::write_output(finalOutputPath, finalAggregatedData)) {
            ErrorHandler::reportError("ORCHESTRATOR_DLL_CONTROLLER: Failed to write final aggregated output to " + finalOutputPath, true);
            return false;
        }
        Logger::getInstance().log("ORCHESTRATOR_DLL_CONTROLLER: Final aggregated output written to " + finalOutputPath);
        return true;
    }
};

#endif // PROCESS_ORCHESTRATOR_DLL_H
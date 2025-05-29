#ifndef PROCESS_ORCHESTRATOR_DLL_H
#define PROCESS_ORCHESTRATOR_DLL_H

#include <string>
#include <vector>
#include <map>
#include <iostream> 
#include <cstdlib>  
#include <sstream>  
#include <algorithm>
#include <filesystem> 

// Project Headers - Adjust paths if these are not directly in the include path
#include "FileHandler.h"   
#include "Logger.h"        
#include "ErrorHandler.h"  
#include "Mapper_DLL_so.h" 
#include "Reducer_DLL_so.h"

// Export macro (optional, but good practice if you ever compile this to a separate DLL)
#if defined(_WIN32) || defined(_WIN64)
#define ORCH_DLL_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define ORCH_DLL_EXPORT __attribute__((visibility("default")))
#else
#define ORCH_DLL_EXPORT
#endif

namespace fs = std::filesystem;

class ORCH_DLL_EXPORT ProcessOrchestratorDLL {
public:
    virtual ~ProcessOrchestratorDLL() {}

    virtual int launchProcess(const std::string& command) {
        Logger::getInstance().log("ORCH_DLL: Executing command: " + command);
        int result = std::system(command.c_str()); 
        if (result != 0) {
            ErrorHandler::reportError("ORCH_DLL: Command returned non-zero (" + std::to_string(result) + "): " + command);
        }
        return result; 
    }

    virtual bool runController(const std::string& executablePath,
                               const std::string& inputDir,
                               const std::string& outputDir,
                               const std::string& tempDir,
                               int numMappers,
                               int numReducers) {
        Logger::getInstance().log("ORCH_DLL_CTRL: Controller starting. M=" + std::to_string(numMappers) + ", R=" + std::to_string(numReducers));

        if (numMappers <= 0 || numReducers <= 0) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Number of mappers and reducers must be positive.", true);
            return false;
        }

        if (!setupDirectories_impl(inputDir, outputDir, tempDir)) {
            return false;
        }

        std::vector<std::vector<std::string>> mapperFileAssignments;
        if (!distributeInputFiles_impl(inputDir, numMappers, mapperFileAssignments)) {
            return false; 
        }
        
        bool noInputFiles = true;
        for(const auto& files : mapperFileAssignments) {
            if (!files.empty()) {
                noInputFiles = false;
                break;
            }
        }

        if (noInputFiles) {
             Logger::getInstance().log("ORCH_DLL_CTRL: No input files to process after distribution.");
             // Use '/' for path construction for consistency in this part
             if (!FileHandler::create_empty_file(outputDir + "/_SUCCESS")) return false;
             Logger::getInstance().log("ORCH_DLL_CTRL: MapReduce job completed successfully (no input files).");
             return true;
        }

        if (!launchMapperProcesses_impl(executablePath, tempDir, numMappers, numReducers, mapperFileAssignments)) {
            return false;
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: All mapper processes launched (assumed complete due to synchronous std::system).");

        if (!launchReducerProcesses_impl(executablePath, outputDir, tempDir, numReducers)) {
            return false;
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: All reducer processes launched (assumed complete due to synchronous std::system).");

        if (!performFinalAggregation_impl(outputDir, numReducers, "final_result.txt")) {
            return false;
        }
        // Use '/' for path construction
        if (!FileHandler::create_empty_file(outputDir + "/_SUCCESS")) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to create _SUCCESS file.", true);
            return false;
        }

        Logger::getInstance().log("ORCH_DLL_CTRL: MapReduce job completed successfully.");
        return true;
    }

    virtual bool runMapper(const std::string& tempDir,
                           int mapperId,
                           int numReducers,
                           const std::vector<std::string>& inputFilePaths) {
        Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + " starting for " + std::to_string(inputFilePaths.size()) + " files.");

        MapperDLLso mapperInstance; 
        std::vector<std::string> allLines;

        for (const auto& filePath : inputFilePaths) {
            Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Processing file " + filePath);
            std::vector<std::string> linesForFile;
            // Assumes FileHandler::read_file exists and works as expected
            if (!FileHandler::read_file(filePath, linesForFile)) {
                ErrorHandler::reportError("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Failed to read input file " + filePath, true);
                return false;
            }
            allLines.insert(allLines.end(), linesForFile.begin(), linesForFile.end());
        }

        if (allLines.empty() && !inputFilePaths.empty()) {
            Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": All assigned input files were empty.");
        } else if (inputFilePaths.empty()) {
             Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": No input files assigned.");
             return true; 
        }
        
        Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Executing map_words for " + std::to_string(allLines.size()) + " total lines.");
        try {
            // CRITICAL ASSUMPTION: Mapper_DLL_so.h has a map_words overload like:
            // map_words(const std::vector<std::string>& lines, const std::string& tempDir, int mapperId, int numReducers)
            mapperInstance.map_words(allLines, tempDir, mapperId, numReducers);
        } catch (const std::exception& e) {
            ErrorHandler::reportError("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Exception during map_words: " + std::string(e.what()), true);
            return false;
        }

        Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + " finished successfully.");
        return true;
    }

    virtual bool runReducer(const std::string& outputDir,
                            const std::string& tempDir,
                            int reducerId) {
        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + " starting.");

        ReducerDLLso reducerInstance; 
        std::vector<std::pair<std::string, int>> allMappedDataForPartition;

        std::vector<std::string> allTempFilesPaths;
        // CRITICAL ASSUMPTION: FileHandler.h has get_files_in_directory(const std::string& dirPath, std::vector<std::string>& filePaths, const std::string& extensionFilter = "")
        if (!FileHandler::get_files_in_directory(tempDir, allTempFilesPaths)) {
            ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Could not list files in temp directory " + tempDir, true);
            return false;
        }

        std::string partitionFileSuffix = "_partition" + std::to_string(reducerId) + ".tmp";
        for (const auto& tempFilePathStr : allTempFilesPaths) {
            fs::path tempFileFsPath(tempFilePathStr); 
            std::string filename = tempFileFsPath.filename().string();

            if (filename.rfind("mapper", 0) == 0 &&
                filename.length() > partitionFileSuffix.length() &&
                filename.substr(filename.length() - partitionFileSuffix.length()) == partitionFileSuffix)
            {
                Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Reading intermediate file " + tempFilePathStr);
                // Assumes FileHandler::read_mapped_data works as expected
                if (!FileHandler::read_mapped_data(tempFilePathStr, allMappedDataForPartition)) {
                    ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Failed to read or parse intermediate file " + tempFilePathStr + ". This may lead to incomplete results.", false);
                }
            }
        }
        
        if (allMappedDataForPartition.empty()) {
            Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": No data found for this partition after reading intermediate files.");
            std::map<std::string, int> emptyData;
            // Use '/' for path construction
            std::string outputFilePath = outputDir + "/result_partition" + std::to_string(reducerId) + ".txt";
            if (!FileHandler::write_output(outputFilePath, emptyData)) {
                 ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Failed to write empty output file " + outputFilePath, true);
                 return false;
            }
            Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + " finished successfully (no data).");
            return true;
        }

        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Sorting " + std::to_string(allMappedDataForPartition.size()) + " key-value pairs.");
        std::sort(allMappedDataForPartition.begin(), allMappedDataForPartition.end(), 
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                      return a.first < b.first;
                  });

        std::map<std::string, int> reducedData;
        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Executing reduce operation.");
        try {
            reducerInstance.reduce(allMappedDataForPartition, reducedData);
        } catch (const std::exception& e) {
            ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Exception during reduce: " + std::string(e.what()), true);
            return false;
        }
        // Use '/' for path construction
        std::string outputFilePath = outputDir + "/result_partition" + std::to_string(reducerId) + ".txt";
        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Writing final output to " + outputFilePath);
        if (!FileHandler::write_output(outputFilePath, reducedData)) {
            ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Failed to write final output file " + outputFilePath, true);
            return false;
        }

        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + " finished successfully.");
        return true;
    }

private:
    std::string buildCommandString_impl(
        const std::string& executablePath, 
        const std::string& mode, 
        const std::vector<std::string>& args,
        const std::vector<std::string>& fileArgs = {}) const
    {
        std::ostringstream cmd;
        cmd << "\"" << executablePath << "\" " << mode; 
        for (const auto& arg : args) {
            cmd << " \"" << arg << "\""; 
        }
        for (const auto& fileArg : fileArgs) {
            cmd << " \"" << fileArg << "\""; 
        }
        return cmd.str();
    }

    bool setupDirectories_impl(const std::string& inputDir, const std::string& outputDir, const std::string& tempDir) const {
        Logger::getInstance().log("ORCH_DLL_CTRL: Setting up directories...");
        // CRITICAL ASSUMPTION: FileHandler.h has a validate_directory overload like:
        // validate_directory(const std::string& path, bool create_if_missing)
        if (!FileHandler::validate_directory(inputDir, false)) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Input directory " + inputDir + " is not valid or does not exist.", true);
            return false;
        }
        if (!FileHandler::validate_directory(outputDir, true)) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to validate/create output directory " + outputDir, true);
            return false;
        }
        if (!FileHandler::validate_directory(tempDir, true)) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to validate/create temporary directory " + tempDir, true);
            return false;
        }
        return true;
    }

    bool distributeInputFiles_impl(const std::string& inputDir, int numMappers, std::vector<std::vector<std::string>>& mapperFileAssignments) const {
        Logger::getInstance().log("ORCH_DLL_CTRL: Distributing input files from " + inputDir);
        std::vector<std::string> allInputFiles;
        // CRITICAL ASSUMPTION: FileHandler.h has get_files_in_directory(const std::string& dirPath, std::vector<std::string>& filePaths, const std::string& extensionFilter = "")
        if (!FileHandler::get_files_in_directory(inputDir, allInputFiles, ".txt")) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to get input files from " + inputDir);
            return false;
        }

        if (allInputFiles.empty()) {
            Logger::getInstance().log("ORCH_DLL_CTRL: No input files found in " + inputDir + ". Nothing to map.");
            return true; 
        }

        mapperFileAssignments.assign(numMappers, std::vector<std::string>());
        for (size_t i = 0; i < allInputFiles.size(); ++i) {
            mapperFileAssignments[i % numMappers].push_back(allInputFiles[i]);
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: Distributed " + std::to_string(allInputFiles.size()) + " files among " + std::to_string(numMappers) + " mappers.");
        return true;
    }

    bool launchMapperProcesses_impl(
        const std::string& executablePath, 
        const std::string& tempDir, 
        int numMappers, 
        int numReducers,
        const std::vector<std::vector<std::string>>& mapperFileAssignments)
    {
        Logger::getInstance().log("ORCH_DLL_CTRL: Launching " + std::to_string(numMappers) + " mapper processes...");
        for (int i = 0; i < numMappers; ++i) {
            if (mapperFileAssignments[i].empty()) {
                Logger::getInstance().log("ORCH_DLL_CTRL: Mapper " + std::to_string(i) + " has no files assigned, skipping launch.");
                continue;
            }
            std::vector<std::string> args;
            args.push_back(tempDir);
            args.push_back(std::to_string(i)); 
            args.push_back(std::to_string(numReducers));
            args.push_back("DEFAULT"); // Log file path placeholder for main.cpp to interpret

            std::string cmd = buildCommandString_impl(executablePath, "mapper", args, mapperFileAssignments[i]);
            
            if (this->launchProcess(cmd) != 0) { 
                ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to launch or error in mapper " + std::to_string(i) + ". Command: " + cmd, true);
                return false;
            }
        }
        return true;
    }
    
    bool launchReducerProcesses_impl(
        const std::string& executablePath,
        const std::string& outputDir,
        const std::string& tempDir,
        int numReducers)
    {
        Logger::getInstance().log("ORCH_DLL_CTRL: Launching " + std::to_string(numReducers) + " reducer processes...");
        for (int i = 0; i < numReducers; ++i) {
            std::vector<std::string> args;
            args.push_back(outputDir);
            args.push_back(tempDir);
            args.push_back(std::to_string(i)); 
            args.push_back("DEFAULT"); 

            std::string cmd = buildCommandString_impl(executablePath, "reducer", args);
            if (this->launchProcess(cmd) != 0) { 
                ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to launch or error in reducer " + std::to_string(i) + ". Command: " + cmd, true);
                return false;
            }
        }
        return true;
    }

    bool performFinalAggregation_impl(const std::string& outputDir, int numReducers, const std::string& finalOutputFilename) const {
        Logger::getInstance().log("ORCH_DLL_CTRL: Performing final aggregation of reducer outputs...");
        std::map<std::string, int> finalAggregatedData;
        bool anyReducerOutputFound = false;

        for (int i = 0; i < numReducers; ++i) {
            // Use '/' for path construction
            std::string reducerOutputFile = outputDir + "/result_partition" + std::to_string(i) + ".txt";
            if (!fs::exists(reducerOutputFile)) {
                Logger::getInstance().log("ORCH_DLL_CTRL: Reducer output file " + reducerOutputFile + " not found. Skipping.");
                continue; 
            }
            anyReducerOutputFound = true;
            std::vector<std::pair<std::string, int>> reducerPartitionData;
            if (FileHandler::read_mapped_data(reducerOutputFile, reducerPartitionData)) {
                for (const auto& pairVal : reducerPartitionData) { 
                    finalAggregatedData[pairVal.first] += pairVal.second;
                }
            } else {
                ErrorHandler::reportError("ORCH_DLL_CTRL: Issues reading reducer output file: " + reducerOutputFile + ". Aggregation may be incomplete.", false); 
            }
        }
        
        if (!anyReducerOutputFound && numReducers > 0) {
            Logger::getInstance().log("ORCH_DLL_CTRL: No reducer output files found for aggregation. Final result will be empty.");
        }
        // Use '/' for path construction
        std::string finalOutputPath = outputDir + "/" + finalOutputFilename;
        if (!FileHandler::write_output(finalOutputPath, finalAggregatedData)) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to write final aggregated output to " + finalOutputPath, true);
            return false;
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: Final aggregated output written to " + finalOutputPath);
        return true;
    }
};

#endif // PROCESS_ORCHESTRATOR_DLL_H
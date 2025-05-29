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
#include <thread> // For hardware_concurrency

#include "FileHandler.h"   
#include "Logger.h"        
#include "ErrorHandler.h"  
#include "Mapper_DLL_so.h" 
#include "Reducer_DLL_so.h"

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
    // Default thread counts if not specified by user
    static constexpr size_t DEFAULT_MIN_THREADS = 0; // Indicates to use hardware_concurrency or a fallback
    static constexpr size_t DEFAULT_MAX_THREADS = 0; // Indicates to use hardware_concurrency or a fallback
    static constexpr size_t FALLBACK_THREAD_COUNT = 2; // Fallback if hardware_concurrency is 0


    virtual ~ProcessOrchestratorDLL() {}

    virtual int launchProcess(const std::string& command) {
        Logger::getInstance().log("ORCH_DLL: Executing command: " + command);
        int result = std::system(command.c_str()); 
        if (result != 0) {
            ErrorHandler::reportError("ORCH_DLL: Command returned non-zero (" + std::to_string(result) + "): " + command);
        }
        return result; 
    }

    // Updated runController signature
    virtual bool runController(const std::string& executablePath,
                               const std::string& inputDir,
                               const std::string& outputDir,
                               const std::string& tempDir,
                               int numMappers,
                               int numReducers,
                               size_t mapperMinPoolThreads = DEFAULT_MIN_THREADS,
                               size_t mapperMaxPoolThreads = DEFAULT_MAX_THREADS,
                               size_t reducerMinPoolThreads = DEFAULT_MIN_THREADS,
                               size_t reducerMaxPoolThreads = DEFAULT_MAX_THREADS) {
        Logger::getInstance().log("ORCH_DLL_CTRL: Controller starting. M=" + std::to_string(numMappers) + ", R=" + std::to_string(numReducers));
        Logger::getInstance().log("ORCH_DLL_CTRL: Mapper Pool Threads: " + formatThreadCount(mapperMinPoolThreads) + "-" + formatThreadCount(mapperMaxPoolThreads));
        Logger::getInstance().log("ORCH_DLL_CTRL: Reducer Pool Threads: " + formatThreadCount(reducerMinPoolThreads) + "-" + formatThreadCount(reducerMaxPoolThreads));


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
             if (!FileHandler::create_empty_file( (fs::path(outputDir) / "_SUCCESS").string() )) return false;
             Logger::getInstance().log("ORCH_DLL_CTRL: MapReduce job completed successfully (no input files).");
             return true;
        }

        if (!launchMapperProcesses_impl(executablePath, tempDir, numMappers, numReducers, mapperFileAssignments, mapperMinPoolThreads, mapperMaxPoolThreads)) {
            return false;
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: All mapper processes launched (assumed complete).");

        if (!launchReducerProcesses_impl(executablePath, outputDir, tempDir, numReducers, reducerMinPoolThreads, reducerMaxPoolThreads)) {
            return false;
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: All reducer processes launched (assumed complete).");

        if (!performFinalAggregation_impl(outputDir, numReducers, "final_result.txt")) {
            return false;
        }
        if (!FileHandler::create_empty_file( (fs::path(outputDir) / "_SUCCESS").string() )) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to create _SUCCESS file.", true);
            return false;
        }

        Logger::getInstance().log("ORCH_DLL_CTRL: MapReduce job completed successfully.");
        return true;
    }

    // Updated runMapper signature
    virtual bool runMapper(const std::string& tempDir,
                           int mapperId,
                           int numReducers,
                           const std::vector<std::string>& inputFilePaths,
                           size_t minPoolThreads = DEFAULT_MIN_THREADS,
                           size_t maxPoolThreads = DEFAULT_MAX_THREADS) {
        
        size_t actualMinThreads = (minPoolThreads == DEFAULT_MIN_THREADS) ? resolveDefaultThreads() : minPoolThreads;
        size_t actualMaxThreads = (maxPoolThreads == DEFAULT_MAX_THREADS) ? actualMinThreads : maxPoolThreads; // if max is default, make it same as min
        if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;


        Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + " starting for " + 
                                  std::to_string(inputFilePaths.size()) + " files. Pool: " +
                                  std::to_string(actualMinThreads) + "-" + std::to_string(actualMaxThreads) + " threads.");

        MapperDLLso mapperInstance; 
        std::vector<std::string> allLines;

        for (const auto& filePath : inputFilePaths) {
            Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Processing file " + filePath);
            std::vector<std::string> linesForFile;
            if (!FileHandler::read_file(filePath, linesForFile)) {
                ErrorHandler::reportError("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Failed to read input file " + filePath, true);
                return false;
            }
            allLines.insert(allLines.end(), linesForFile.begin(), linesForFile.end());
        }

        if (allLines.empty() && !inputFilePaths.empty()) {
            Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": All assigned input files were empty.");
        } else if (inputFilePaths.empty()) {
             Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": No input files assigned. Completing successfully.");
             return true; 
        }
        
        Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Executing map_words for " + std::to_string(allLines.size()) + " total lines.");
        try {
            // Call the 6-argument map_words
            mapperInstance.map_words(allLines, tempDir, mapperId, numReducers, actualMinThreads, actualMaxThreads);
        } catch (const std::exception& e) {
            ErrorHandler::reportError("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + ": Exception during map_words: " + std::string(e.what()), true);
            return false;
        }

        Logger::getInstance().log("ORCH_DLL_MAP: Mapper " + std::to_string(mapperId) + " finished successfully.");
        return true;
    }

    // Updated runReducer signature
    virtual bool runReducer(const std::string& outputDir,
                            const std::string& tempDir,
                            int reducerId,
                            size_t minPoolThreads = DEFAULT_MIN_THREADS,
                            size_t maxPoolThreads = DEFAULT_MAX_THREADS) {

        size_t actualMinThreads = (minPoolThreads == DEFAULT_MIN_THREADS) ? resolveDefaultThreads() : minPoolThreads;
        size_t actualMaxThreads = (maxPoolThreads == DEFAULT_MAX_THREADS) ? actualMinThreads : maxPoolThreads;
        if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;

        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + " starting. Pool: " +
                                  std::to_string(actualMinThreads) + "-" + std::to_string(actualMaxThreads) + " threads.");

        ReducerDLLso reducerInstance; 
        std::vector<std::pair<std::string, int>> allMappedDataForPartition;
        std::vector<std::string> allTempFilesPaths;

        if (!FileHandler::get_files_in_directory(tempDir, allTempFilesPaths)) { // No extension filter, get all files
            ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Could not list files in temp directory " + tempDir, true);
            return false;
        }

        std::string partitionFileSuffix = "_partition" + std::to_string(reducerId) + ".tmp";
        for (const auto& tempFilePathStr : allTempFilesPaths) {
            fs::path tempFileFsPath(tempFilePathStr); 
            std::string filename = tempFileFsPath.filename().string();

            // Check if filename starts with "mapper" and ends with the correct partition suffix
            if (filename.rfind("mapper", 0) == 0 && // starts with "mapper"
                filename.length() > partitionFileSuffix.length() &&
                filename.substr(filename.length() - partitionFileSuffix.length()) == partitionFileSuffix)
            {
                Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Reading intermediate file " + tempFilePathStr);
                if (!FileHandler::read_mapped_data(tempFilePathStr, allMappedDataForPartition)) {
                    ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Failed to read/parse " + tempFilePathStr + ". Results may be incomplete.", false);
                }
            }
        }
        
        if (allMappedDataForPartition.empty()) {
            Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": No data found for this partition.");
            std::map<std::string, int> emptyData;
            std::string outputFilePath = (fs::path(outputDir) / ("result_partition" + std::to_string(reducerId) + ".txt")).string();
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
            // Call the 4-argument reduce
            reducerInstance.reduce(allMappedDataForPartition, reducedData, actualMinThreads, actualMaxThreads);
        } catch (const std::exception& e) {
            ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Exception during reduce: " + std::string(e.what()), true);
            return false;
        }
        
        std::string outputFilePath = (fs::path(outputDir) / ("result_partition" + std::to_string(reducerId) + ".txt")).string();
        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Writing final output to " + outputFilePath);
        if (!FileHandler::write_output(outputFilePath, reducedData)) {
            ErrorHandler::reportError("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + ": Failed to write final output file " + outputFilePath, true);
            return false;
        }

        Logger::getInstance().log("ORCH_DLL_RED: Reducer " + std::to_string(reducerId) + " finished successfully.");
        return true;
    }

private:
    size_t resolveDefaultThreads() const {
        unsigned int hw_threads = std::thread::hardware_concurrency();
        return (hw_threads > 0) ? hw_threads : FALLBACK_THREAD_COUNT;
    }
    std::string formatThreadCount(size_t count) const {
        if (count == DEFAULT_MIN_THREADS || count == DEFAULT_MAX_THREADS) return "Default";
        return std::to_string(count);
    }

    std::string buildCommandString_impl(
        const std::string& executablePath, 
        const std::string& mode, 
        const std::vector<std::string>& args,
        const std::vector<std::string>& fileArgs = {}) const
    {
        std::ostringstream cmd;
        // Ensure executablePath is quoted if it contains spaces
        cmd << "\"" << executablePath << "\" " << mode; 
        for (const auto& arg : args) {
            // Ensure each argument is quoted
            cmd << " \"" << arg << "\""; 
        }
        for (const auto& fileArg : fileArgs) {
            cmd << " \"" << fileArg << "\""; 
        }
        return cmd.str();
    }

    bool setupDirectories_impl(const std::string& inputDir, const std::string& outputDir, const std::string& tempDir) const {
        Logger::getInstance().log("ORCH_DLL_CTRL: Setting up directories...");
        if (!FileHandler::validate_directory(inputDir, false)) { // Input dir must exist, don't create
            ErrorHandler::reportError("ORCH_DLL_CTRL: Input directory " + inputDir + " is not valid or does not exist.", true);
            return false;
        }
        if (!FileHandler::validate_directory(outputDir, true)) { // Output dir can be created
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to validate/create output directory " + outputDir, true);
            return false;
        }
        if (!FileHandler::validate_directory(tempDir, true)) { // Temp dir can be created
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to validate/create temporary directory " + tempDir, true);
            return false;
        }
        return true;
    }

    bool distributeInputFiles_impl(const std::string& inputDir, int numMappers, std::vector<std::vector<std::string>>& mapperFileAssignments) const {
        Logger::getInstance().log("ORCH_DLL_CTRL: Distributing input files from " + inputDir);
        std::vector<std::string> allInputFiles;
        if (!FileHandler::get_files_in_directory(inputDir, allInputFiles, ".txt")) { // Filter for .txt files
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to get .txt input files from " + inputDir);
            // It's not necessarily an error if no .txt files are found, could be other types or empty.
            // The function get_files_in_directory should return true if directory was read, even if no matching files.
            // Let's assume it returns true and allInputFiles is empty if no .txt files.
        }

        if (allInputFiles.empty()) {
            Logger::getInstance().log("ORCH_DLL_CTRL: No .txt input files found in " + inputDir + ". Nothing to map.");
            // This is not an error condition for distributeInputFiles_impl itself.
            // The caller (runController) will handle this.
            return true; 
        }

        mapperFileAssignments.assign(numMappers, std::vector<std::string>());
        for (size_t i = 0; i < allInputFiles.size(); ++i) {
            mapperFileAssignments[i % numMappers].push_back(allInputFiles[i]);
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: Distributed " + std::to_string(allInputFiles.size()) + " .txt files among " + std::to_string(numMappers) + " mappers.");
        return true;
    }

    // Updated launchMapperProcesses_impl signature
    bool launchMapperProcesses_impl(
        const std::string& executablePath, 
        const std::string& tempDir, 
        int numMappers, 
        int numReducers,
        const std::vector<std::vector<std::string>>& mapperFileAssignments,
        size_t minPoolThreads, 
        size_t maxPoolThreads)
    {
        Logger::getInstance().log("ORCH_DLL_CTRL: Launching " + std::to_string(numMappers) + " mapper processes...");
        for (int i = 0; i < numMappers; ++i) {
            if (mapperFileAssignments[i].empty()) {
                Logger::getInstance().log("ORCH_DLL_CTRL: Mapper " + std::to_string(i) + " has no files assigned, skipping launch.");
                continue;
            }
            std::vector<std::string> args;
            args.push_back(tempDir);
            args.push_back(std::to_string(i)); // mapperId
            args.push_back(std::to_string(numReducers));
            args.push_back(std::to_string(minPoolThreads)); // Pass thread config
            args.push_back(std::to_string(maxPoolThreads)); // Pass thread config
            args.push_back( (fs::path(tempDir) / ("mapper_" + std::to_string(i) + ".log")).string() ); // Log file path

            std::string cmd = buildCommandString_impl(executablePath, "mapper", args, mapperFileAssignments[i]);
            
            if (this->launchProcess(cmd) != 0) { 
                ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to launch or error in mapper " + std::to_string(i) + ". Command: " + cmd, true);
                return false; // Stop if any mapper fails to launch or errors out
            }
        }
        return true;
    }
    
    // Updated launchReducerProcesses_impl signature
    bool launchReducerProcesses_impl(
        const std::string& executablePath,
        const std::string& outputDir,
        const std::string& tempDir,
        int numReducers,
        size_t minPoolThreads,
        size_t maxPoolThreads)
    {
        Logger::getInstance().log("ORCH_DLL_CTRL: Launching " + std::to_string(numReducers) + " reducer processes...");
        for (int i = 0; i < numReducers; ++i) {
            std::vector<std::string> args;
            args.push_back(outputDir);
            args.push_back(tempDir);
            args.push_back(std::to_string(i)); // reducerId
            args.push_back(std::to_string(minPoolThreads)); // Pass thread config
            args.push_back(std::to_string(maxPoolThreads)); // Pass thread config
            args.push_back( (fs::path(tempDir) / ("reducer_" + std::to_string(i) + ".log")).string() ); // Log file path

            std::string cmd = buildCommandString_impl(executablePath, "reducer", args);
            if (this->launchProcess(cmd) != 0) { 
                ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to launch or error in reducer " + std::to_string(i) + ". Command: " + cmd, true);
                return false; // Stop if any reducer fails
            }
        }
        return true;
    }

    bool performFinalAggregation_impl(const std::string& outputDir, int numReducers, const std::string& finalOutputFilename) const {
        Logger::getInstance().log("ORCH_DLL_CTRL: Performing final aggregation of reducer outputs...");
        std::map<std::string, int> finalAggregatedData;
        bool anyReducerOutputFound = false;

        for (int i = 0; i < numReducers; ++i) {
            std::string reducerOutputFile = (fs::path(outputDir) / ("result_partition" + std::to_string(i) + ".txt")).string();
            if (!fs::exists(reducerOutputFile)) {
                Logger::getInstance().log("ORCH_DLL_CTRL: Reducer output file " + reducerOutputFile + " not found. Skipping.");
                continue; 
            }
            anyReducerOutputFound = true; // Mark that we found at least one output
            std::vector<std::pair<std::string, int>> reducerPartitionData;
            if (FileHandler::read_mapped_data(reducerOutputFile, reducerPartitionData)) {
                for (const auto& pairVal : reducerPartitionData) { 
                    finalAggregatedData[pairVal.first] += pairVal.second;
                }
            } else {
                // Log error but continue, to aggregate as much as possible
                ErrorHandler::reportError("ORCH_DLL_CTRL: Issues reading reducer output file: " + reducerOutputFile + ". Aggregation may be incomplete.", false); 
            }
        }
        
        if (!anyReducerOutputFound && numReducers > 0) {
            Logger::getInstance().log("ORCH_DLL_CTRL: No reducer output files found for aggregation. Final result will be empty.");
        }
        
        std::string finalOutputPath = (fs::path(outputDir) / finalOutputFilename).string();
        if (!FileHandler::write_output(finalOutputPath, finalAggregatedData)) {
            ErrorHandler::reportError("ORCH_DLL_CTRL: Failed to write final aggregated output to " + finalOutputPath, true);
            return false;
        }
        Logger::getInstance().log("ORCH_DLL_CTRL: Final aggregated output written to " + finalOutputPath);
        return true;
    }
};

#endif // PROCESS_ORCHESTRATOR_DLL_H
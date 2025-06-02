#ifdef _WIN32
    #include "..\include\ERROR_Handler.h"
    #include "..\include\FileHandler.h"
    #include "..\include\Logger.h"
    #include "..\include\Mapper_DLL_so.h" 
    #include "..\include\Reducer_DLL_so.h" 
    #include "..\include\ProcessOrchestrator.h"
    #include "..\include\InteractiveMode.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/ERROR_Handler.h"
    #include "../include/FileHandler.h"
    #include "../include/Logger.h"
    #include "../include/Mapper_DLL_so.h" 
    #include "../include/Reducer_DLL_so.h" 
    #include "../include/ProcessOrchestrator.h"
    #include "../include/InteractiveMode.h"
#else
    #error "Unsupported operating system. Please utilize Windows, MacOS, or any Linux distribution to operate this C++ program."
#endif

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <algorithm> 
#include <cctype>    

namespace fs = std::filesystem;

enum class AppMode {
    CONTROLLER,
    MAPPER,
    REDUCER,
    INTERACTIVE,
    UNKNOWN
};

AppMode parseMode(const std::string& modeStr) {
    std::string lowerModeStr = modeStr;
    std::transform(lowerModeStr.begin(), lowerModeStr.end(), lowerModeStr.begin(),
                   [](unsigned char c){ return std::tolower(c); }); 
    if (lowerModeStr == "controller") return AppMode::CONTROLLER;
    if (lowerModeStr == "mapper") return AppMode::MAPPER;
    if (lowerModeStr == "reducer") return AppMode::REDUCER;
    if (lowerModeStr == "interactive") return AppMode::INTERACTIVE;
    return AppMode::UNKNOWN;
}

void signalReducers(std::condition_variable &cv, std::mutex &mtx, bool &readyFlag) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        readyFlag = true;
    }
    cv.notify_all();
    Logger::getInstance().log("CONTROLLER: Signaled reducers that mapper outputs are ready.");
}

// int runInteractiveWorkflow(); // Ensure this is declared if defined in another .cpp without a header

int main(int argc, char* argv[]) {
    AppMode currentMode = AppMode::INTERACTIVE;
    // std::string executablePath = argv[0]; // Unused variable
    bool cmdModeSuccess = false;

    Logger& logger = Logger::getInstance();
    logger.configureLogFilePath("MapReduce.log");
    logger.setPrefix("[MAIN] ");

    ProcessOrchestratorDLL orchestrator;

    if (argc > 1) {
        std::string modeStr = argv[1];
        AppMode detectedMode = parseMode(modeStr);

        if (detectedMode != AppMode::UNKNOWN) {
            currentMode = detectedMode;
            logger.log("Application mode detected: " + modeStr);

            try {
                switch (currentMode) {
                    case AppMode::CONTROLLER: {
                        if (argc < 7) {
                            ErrorHandler::reportError("Controller usage: " + std::string(argv[0]) + " controller <inputDir> <outputDir> <tempDir> <M> <R> [<successFileName>] [<finalOutputName>] [<partitionPrefix>] [<partitionSuffix>]", true);
                        }
                        std::string inputDir = argv[2];
                        std::string outputDir = argv[3];
                        std::string tempDir = argv[4];
                        int numMappers = 0;
                        int numReducers = 0;
                        try {
                            numMappers = std::stoi(argv[5]);
                            numReducers = std::stoi(argv[6]);
                        } catch (const std::invalid_argument& ia) {
                            ErrorHandler::reportError("Invalid number for Mappers or Reducers. M=" + std::string(argv[5]) + ", R=" + std::string(argv[6]) + ". Error: " + ia.what(), true);
                        } catch (const std::out_of_range& oor) {
                             ErrorHandler::reportError("Number for Mappers or Reducers out of range. M=" + std::string(argv[5]) + ", R=" + std::string(argv[6]) + ". Error: " + oor.what(), true);
                        }

                        if (numMappers <= 0 || numReducers <= 0) {
                             ErrorHandler::reportError("Number of Mappers (M) and Reducers (R) must be positive.", true);
                        }
                        
                        std::string successFileName = (argc > 7) ? argv[7] : "SUCCESS.txt";
                        std::string finalOutputName = (argc > 8) ? argv[8] : "final_results.txt";
                        std::string partitionPrefix = (argc > 9) ? argv[9] : "partition_";
                        std::string partitionSuffix = (argc > 10) ? argv[10] : ".txt";

                        logger.log("Controller Config: inputDir=" + inputDir + ", outputDir=" + outputDir + ", tempDir=" + tempDir +
                                   ", M=" + std::to_string(numMappers) + ", R=" + std::to_string(numReducers) +
                                   ", successFile=" + successFileName + ", finalOutput=" + finalOutputName +
                                   ", partitionPrefix=" + partitionPrefix + ", partitionSuffix=" + partitionSuffix);

                        std::vector<std::string> allInputFiles;
                        if (!FileHandler::validate_directory(inputDir, allInputFiles, inputDir, false)) {
                            ErrorHandler::reportError("Failed to validate input directory or read files from: " + inputDir, true);
                        }
                        // CORRECTED LINE 127 (based on your log)
                        if (allInputFiles.empty()) {
                            logger.log("No input files found in " + inputDir + ". Nothing to map.", Logger::Level::WARNING); 
                        } else {
                             logger.log("Found " + std::to_string(allInputFiles.size()) + " input files.");
                        }

                        std::condition_variable cvReducers;
                        std::mutex mtxReducers;
                        bool mapperOutputsReady = false;

                        std::vector<std::thread> mapperThreads;
                        std::vector<std::vector<std::string>> mapperFileAssignments(numMappers);

                        for (size_t i = 0; i < allInputFiles.size(); ++i) {
                            if (numMappers > 0) { 
                                mapperFileAssignments[i % numMappers].push_back(allInputFiles[i]);
                            }
                        }
                        
                        logger.log("CONTROLLER: Launching " + std::to_string(numMappers) + " mapper processes/threads.");
                        for (int i = 0; i < numMappers; ++i) {
                            mapperThreads.emplace_back([&logger, &orchestrator, tempDir, i, numReducers, files = mapperFileAssignments[i], partitionPrefix, partitionSuffix]() mutable { 
                                logger.log("CONTROLLER: Starting mapper thread/process " + std::to_string(i) + " with " + std::to_string(files.size()) + " files.");
                                orchestrator.runMapper(tempDir, i, numReducers, files, 2, 4); // Assuming ProcessOrchestratorDLL::runMapper is updated
                                logger.log("CONTROLLER: Mapper thread/process " + std::to_string(i) + " finished.");
                            });
                        }

                        logger.log("CONTROLLER: Launching " + std::to_string(numReducers) + " reducer processes/threads.");
                        std::vector<std::thread> reducerThreads;
                        for (int i = 0; i < numReducers; ++i) {
                            reducerThreads.emplace_back([&logger, &orchestrator, &cvReducers, &mtxReducers, &mapperOutputsReady, outputDir, tempDir, i]() mutable {
                                logger.log("CONTROLLER: Reducer thread/process " + std::to_string(i) + " created, waiting for mapper signal.");
                                std::unique_lock<std::mutex> lock(mtxReducers); 
                                cvReducers.wait(lock, [&mapperOutputsReady]() { return mapperOutputsReady; }); 
                                logger.log("CONTROLLER: Reducer thread/process " + std::to_string(i) + " received signal, starting reduction.");
                                orchestrator.runReducer(outputDir, tempDir, i, 2, 4); // Assuming ProcessOrchestratorDLL::runReducer is updated
                                logger.log("CONTROLLER: Reducer thread/process " + std::to_string(i) + " finished.");
                            });
                        }
                        logger.log("CONTROLLER: All reducer threads created.");

                        logger.log("CONTROLLER: Waiting for all mapper processes/threads to complete...");
                        for (auto &t : mapperThreads) {
                            if (t.joinable()) t.join();
                        }
                        logger.log("CONTROLLER: All mapper processes/threads completed.");

                        logger.log("CONTROLLER: Initiating distinct sorting step for intermediate data (conceptual).");
                        // orchestrator.performIntermediateSort(tempDir, numReducers, partitionPrefix, partitionSuffix);
                        logger.log("CONTROLLER: Distinct sorting step for intermediate data completed (conceptual).");

                        signalReducers(cvReducers, mtxReducers, mapperOutputsReady);

                        logger.log("CONTROLLER: Waiting for all reducer processes/threads to complete...");
                        for (auto &t : reducerThreads) {
                           if (t.joinable()) t.join();
                        }
                        logger.log("CONTROLLER: All reducer processes/threads completed.");

                        logger.log("CONTROLLER: Performing final reduction/aggregation step.");
                        orchestrator.runFinalReducer(outputDir, tempDir); // Assuming ProcessOrchestratorDLL::runFinalReducer is updated
                        logger.log("CONTROLLER: Final reduction/aggregation step completed.");

                        fs::path successFilePath = fs::path(outputDir) / successFileName;
                        std::ofstream successFileStream(successFilePath);
                        if (successFileStream.is_open()) {
                            successFileStream << "MapReduce job completed successfully.\n";
                            successFileStream << "Timestamp: " << logger.getTimestamp() << "\n"; 
                            successFileStream.close();
                            logger.log("Successfully wrote SUCCESS file: " + successFilePath.string());
                        } else {
                            logger.log("ERROR: Could not write SUCCESS file to " + successFilePath.string(), Logger::Level::ERROR);
                        }

                        cmdModeSuccess = true;
                        break;
                    }
                    case AppMode::MAPPER: {
                        logger.log("Running in MAPPER mode (invoked directly - usually by orchestrator).");
                        if (argc < 7) { 
                            ErrorHandler::reportError("Mapper usage: <executable> mapper <tempDir> <mapperId> <R> [minThreads maxThreads] <mapperLogPath> <inputFile1> [inputFile2 ...]", true);
                        }

                        std::string tempDir = argv[2];
                        int numReducers = std::stoi(argv[4]);

                        size_t minThreads = std::thread::hardware_concurrency();
                        size_t maxThreads = std::thread::hardware_concurrency();
                        std::string logPath;
                        int inputFilesStartIdx = 6;

                        // Detect if thread pool arguments are present
                        if (argc >= 9 && std::all_of(argv[5], argv[5]+strlen(argv[5]), ::isdigit) && std::all_of(argv[6], argv[6]+strlen(argv[6]), ::isdigit)) {
                            minThreads = std::stoul(argv[5]);
                            maxThreads = std::stoul(argv[6]);
                            logPath = argv[7];
                            inputFilesStartIdx = 8;
                        } else {
                            logPath = argv[5];
                            inputFilesStartIdx = 6;
                        }

                        logger.configureLogFilePath(logPath);
                        logger.setPrefix("[MAPPER] ");

                        ErrorHandler errorHandler;

                        Mapper mapper(logger, errorHandler);

                        std::vector<std::pair<std::string, int>> mappedData;
                        for (int i = inputFilesStartIdx; i < argc; ++i) {
                            std::vector<std::string> lines;
                            if (!FileHandler::read_file(argv[i], lines)) {
                                logger.log("Failed to read input file: " + std::string(argv[i]), Logger::Level::ERROR);
                                continue;
                            }
                            for (const auto& line : lines) {
                                mapper.map(argv[i], line, mappedData);
                            }
                        }

                        // Use partitioning
                        std::string partitionPrefix = "partition_";
                        std::string partitionSuffix = ".txt";
                        if (!mapper.exportPartitionedData(tempDir, mappedData, numReducers, partitionPrefix, partitionSuffix)) {
                            logger.log("Mapper failed to export partitioned data.", Logger::Level::ERROR);
                            cmdModeSuccess = false;
                            break;
                        }

                        logger.log("Mapper completed successfully. Partitioned data written to: " + tempDir);
                        cmdModeSuccess = true; 
                        break;
                    }
                    case AppMode::REDUCER: {
                        logger.log("Running in REDUCER mode (invoked directly - usually by orchestrator).");
                        if (argc < 6) { 
                            ErrorHandler::reportError("Reducer usage: <executable> reducer <outputDir> <tempDir> <reducerId> [minThreads maxThreads] <reducerLogPath>", true);
                        }

                        std::string outputDir = argv[2];
                        std::string tempDir = argv[3];
                        int reducerId = std::stoi(argv[4]);

                        size_t minThreads = std::thread::hardware_concurrency();
                        size_t maxThreads = std::thread::hardware_concurrency();
                        std::string logPath;
                        int logArgIdx = 5;

                        // Detect if thread pool arguments are present
                        if (argc >= 8 && std::all_of(argv[5], argv[5]+strlen(argv[5]), ::isdigit) && std::all_of(argv[6], argv[6]+strlen(argv[6]), ::isdigit)) {
                            minThreads = std::stoul(argv[5]);
                            maxThreads = std::stoul(argv[6]);
                            logPath = argv[7];
                            logArgIdx = 7;
                        } else {
                            logPath = argv[5];
                            logArgIdx = 5;
                        }

                        logger.configureLogFilePath(logPath);
                        logger.setPrefix("[REDUCER] ");

                        // Find all partition files for this reducer
                        std::vector<std::string> partitionFiles;
                        for (const auto& entry : fs::directory_iterator(tempDir)) {
                            if (entry.is_regular_file()) {
                                std::string fname = entry.path().filename().string();
                                std::string expected = "partition_" + std::to_string(reducerId) + ".txt";
                                if (fname == expected) {
                                    partitionFiles.push_back(entry.path().string());
                                }
                            }
                        }

                        if (partitionFiles.empty()) {
                            logger.log("No partition files found for reducer " + std::to_string(reducerId) + " in " + tempDir, Logger::Level::WARNING);
                            cmdModeSuccess = true;
                            break;
                        }

                        std::vector<std::pair<std::string, int>> allMappedData;
                        for (const auto& file : partitionFiles) {
                            std::vector<std::pair<std::string, int>> mappedData;
                            FileHandler::read_mapped_data(file, mappedData);
                            allMappedData.insert(allMappedData.end(), mappedData.begin(), mappedData.end());
                        }

                        std::map<std::string, int> reducedData;
                        ReducerDLLso reducer;
                        reducer.reduce(allMappedData, reducedData, minThreads, maxThreads);

                        // Write output
                        fs::create_directories(outputDir);
                        std::string outputPath = (fs::path(outputDir) / ("reducer_" + std::to_string(reducerId) + ".txt")).string();
                        if (!FileHandler::write_output(outputPath, reducedData)) {
                            logger.log("Failed to write reducer output: " + outputPath, Logger::Level::ERROR);
                            cmdModeSuccess = false;
                            break;
                        }

                        logger.log("Reducer completed successfully. Output written to: " + outputPath);
                        cmdModeSuccess = true;
                        break;
                    }
                    default: // Should not happen if parseMode is correct
                        logger.log("Unknown application mode determined internally. Defaulting to interactive.", Logger::Level::ERROR);
                        currentMode = AppMode::INTERACTIVE; // Fallback
                        break;
                }
            } catch (const std::exception &e) {
                logger.log("MAIN_CMD_ERROR: Exception in command mode: " + std::string(e.what()), Logger::Level::ERROR);
                ErrorHandler::reportError("Critical error in command mode: " + std::string(e.what()), false); // false as it might be caught already
                return EXIT_FAILURE;
            }

            logger.log( (cmdModeSuccess ? "Command mode completed successfully." : "Command mode failed.") );
            return cmdModeSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
        } else {
             // CORRECTED LINE 233 (based on your log)
             logger.log("Invalid mode specified: " + std::string(argv[1]) + ". Defaulting to interactive mode.", Logger::Level::WARNING); 
             currentMode = AppMode::INTERACTIVE; // Fallback if mode string is unknown
        }
    }


    if (currentMode == AppMode::INTERACTIVE) {
        logger.log("Starting application in INTERACTIVE mode.");
        // Ensure InteractiveMode.h is included and runInteractiveWorkflow is declared/defined
        return runInteractiveWorkflow(); 
    }

    return EXIT_SUCCESS; 
}
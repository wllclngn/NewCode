#include <iostream>
#include <filesystem> 
#include <fstream>    
#include <sstream>    
#include <map>
#include <vector>
#include <string>
#include <cstdlib> 
#include <stdexcept> // For std::invalid_argument, std::out_of_range

#include "ERROR_Handler.h"     
#include "FileHandler.h"     
#include "Logger.h"          
#include "Mapper_DLL_so.h"   
#include "Reducer_DLL_so.h"  
#include "ProcessOrchestrator.h" 
#include "InteractiveMode.h"   

namespace fs = std::filesystem; 

enum class AppMode {
    CONTROLLER,
    MAPPER,
    REDUCER,
    INTERACTIVE, 
    UNKNOWN
};

AppMode parseMode(const std::string& modeStr) {
    if (modeStr == "controller") return AppMode::CONTROLLER;
    if (modeStr == "mapper") return AppMode::MAPPER;
    if (modeStr == "reducer") return AppMode::REDUCER;
    return AppMode::UNKNOWN;
}

// Helper to safely parse size_t from string for thread counts
size_t parse_size_t(const char* str_val, size_t default_val) {
    if (str_val == nullptr) return default_val;
    std::string s(str_val);
    if (s.empty() || s == "DEFAULT") return default_val;
    try {
        unsigned long long val = std::stoull(s);
        if (val > std::numeric_limits<size_t>::max()) {
            throw std::out_of_range("Value too large for size_t");
        }
        return static_cast<size_t>(val);
    } catch (const std::exception& e) {
        Logger::getInstance().log("MAIN_PARSE_ERROR: Invalid size_t value '" + s + "': " + e.what() + ". Using default.");
        std::cerr << "Warning: Invalid thread count value '" << s << "'. Using default." << std::endl;
        return default_val;
    }
}


int main(int argc, char* argv[]) {
    AppMode currentMode = AppMode::INTERACTIVE;
    std::string executablePath = argv[0];
    bool cmdModeSuccess = false;

    ProcessOrchestratorDLL orchestrator; 

    if (argc > 1) {
        std::string modeStr = argv[1];
        AppMode detectedMode = parseMode(modeStr);

        if (detectedMode != AppMode::UNKNOWN) {
            currentMode = detectedMode; 
            std::string defaultLogFileSuffix = ".log";
            fs::path tempLogDir = fs::temp_directory_path(); // Default log location if others fail

            try {
                switch (currentMode) {
                    case AppMode::CONTROLLER: {
                        // Base args: exec controller inputDir outputDir tempDir M R
                        // Optional: mapMin mapMax reduceMin reduceMax controllerLog
                        if (argc < 7) {
                            std::cerr << "Controller usage: " << argv[0] << " controller <inputDir> <outputDir> <tempDir> <M> <R> "
                                      << "[<mapperMinThreads> <mapperMaxThreads> <reducerMinThreads> <reducerMaxThreads>] [controllerLogPath]" << std::endl;
                            return EXIT_FAILURE;
                        }
                        std::string inputDir = argv[2];
                        std::string outputDir = argv[3];
                        std::string tempDir = argv[4];
                        int numMappers = std::stoi(argv[5]);
                        int numReducers = std::stoi(argv[6]);

                        size_t mapMin = ProcessOrchestratorDLL::DEFAULT_MIN_THREADS;
                        size_t mapMax = ProcessOrchestratorDLL::DEFAULT_MAX_THREADS;
                        size_t redMin = ProcessOrchestratorDLL::DEFAULT_MIN_THREADS;
                        size_t redMax = ProcessOrchestratorDLL::DEFAULT_MAX_THREADS;
                        std::string logPath = (fs::path(outputDir) / ("controller" + defaultLogFileSuffix)).string();

                        int current_arg_idx = 7;
                        if (argc > current_arg_idx + 3) { // Check if all 4 thread args might be present
                            // Try to parse thread counts if they look like numbers
                            try {
                                size_t temp_mapMin = std::stoull(argv[current_arg_idx]); // Test first
                                // If successful, assume all 4 are present
                                mapMin = temp_mapMin;
                                mapMax = parse_size_t(argv[current_arg_idx + 1], mapMin); // Default max to min if invalid
                                redMin = parse_size_t(argv[current_arg_idx + 2], ProcessOrchestratorDLL::DEFAULT_MIN_THREADS);
                                redMax = parse_size_t(argv[current_arg_idx + 3], redMin); // Default max to min if invalid
                                current_arg_idx += 4;
                            } catch (const std::exception&) {
                                // Not thread counts, assume it's the log path next or just fewer args
                                Logger::getInstance().log("MAIN_CTRL_PARSE: Optional thread counts not all provided or invalid, using defaults.");
                            }
                        }
                        if (argc > current_arg_idx) { // Log path
                            logPath = argv[current_arg_idx];
                        }
                         if (logPath == "DEFAULT") logPath = (fs::path(outputDir) / ("controller" + defaultLogFileSuffix)).string();


                        Logger::getInstance().configureLogFilePath(logPath);
                        Logger::getInstance().setPrefix("[CONTROLLER_CMD] ");
                        Logger::getInstance().log("Starting Controller Mode...");
                        
                        cmdModeSuccess = orchestrator.runController(executablePath, inputDir, outputDir, tempDir, numMappers, numReducers,
                                                                  mapMin, mapMax, redMin, redMax);
                        break;
                    }
                    case AppMode::MAPPER: {
                        // Base: exec mapper tempDir mapperId R
                        // Optional: minPool maxPool
                        // Final: logPath inputFile1 [inputFile2 ...]
                        if (argc < 7) { // exec mapper tempDir id R min max log input1
                            std::cerr << "Mapper usage: " << argv[0] << " mapper <tempDir> <mapperId> <R> [<minPoolThreads> <maxPoolThreads>] <logFilePath> <inputFile1> [inputFile2 ...]" << std::endl;
                            return EXIT_FAILURE;
                        }
                        std::string tempDir_m = argv[2];
                        int mapperId = std::stoi(argv[3]);
                        int numReducers_m = std::stoi(argv[4]);
                        
                        size_t minPool_m = ProcessOrchestratorDLL::DEFAULT_MIN_THREADS;
                        size_t maxPool_m = ProcessOrchestratorDLL::DEFAULT_MAX_THREADS;
                        std::string logPath_m;
                        std::vector<std::string> inputFiles_m;
                        int arg_idx_m = 5;

                        // Try to parse thread pool args
                        if (argc > arg_idx_m + 1) { // Potentially min, max, log, file...
                             try {
                                size_t temp_min = std::stoull(argv[arg_idx_m]); // Test first
                                minPool_m = temp_min;
                                maxPool_m = parse_size_t(argv[arg_idx_m+1], minPool_m);
                                arg_idx_m += 2;
                             } catch (const std::exception&) {
                                // Not thread counts, assume next is logPath
                             }
                        }
                        if (argc <= arg_idx_m) { std::cerr << "Mapper: Missing log file path." << std::endl; return EXIT_FAILURE; }
                        logPath_m = argv[arg_idx_m++];
                        if (logPath_m == "DEFAULT") logPath_m = (fs::path(tempDir_m) / ("mapper_" + std::to_string(mapperId) + defaultLogFileSuffix)).string();
                        
                        if (argc <= arg_idx_m) { std::cerr << "Mapper: No input files specified for mapper " << mapperId << "." << std::endl; return EXIT_FAILURE; }
                        for (int i = arg_idx_m; i < argc; ++i) { inputFiles_m.push_back(argv[i]); }

                        Logger::getInstance().configureLogFilePath(logPath_m);
                        Logger::getInstance().setPrefix("[MAPPER_CMD_" + std::to_string(mapperId) + "] ");
                        Logger::getInstance().log("Starting Mapper Mode...");

                        cmdModeSuccess = orchestrator.runMapper(tempDir_m, mapperId, numReducers_m, inputFiles_m, minPool_m, maxPool_m);
                        break;
                    }
                    case AppMode::REDUCER: {
                        // Base: exec reducer outputDir tempDir reducerId
                        // Optional: minPool maxPool
                        // Final: logPath
                        if (argc < 6) { // exec reducer out temp id min max log
                            std::cerr << "Reducer usage: " << argv[0] << " reducer <outputDir> <tempDir> <reducerId> [<minPoolThreads> <maxPoolThreads>] <logFilePath>" << std::endl;
                            return EXIT_FAILURE;
                        }
                        std::string outputDir_r = argv[2];
                        std::string tempDir_r = argv[3];
                        int reducerId = std::stoi(argv[4]);

                        size_t minPool_r = ProcessOrchestratorDLL::DEFAULT_MIN_THREADS;
                        size_t maxPool_r = ProcessOrchestratorDLL::DEFAULT_MAX_THREADS;
                        std::string logPath_r;
                        int arg_idx_r = 5;

                        if (argc > arg_idx_r + 1) { // Potentially min, max, log
                            try {
                                size_t temp_min = std::stoull(argv[arg_idx_r]);
                                minPool_r = temp_min;
                                maxPool_r = parse_size_t(argv[arg_idx_r+1], minPool_r);
                                arg_idx_r += 2;
                            } catch (const std::exception&) {
                                // Not thread counts
                            }
                        }
                        if (argc <= arg_idx_r) { std::cerr << "Reducer: Missing log file path." << std::endl; return EXIT_FAILURE; }
                        logPath_r = argv[arg_idx_r];
                        if (logPath_r == "DEFAULT") logPath_r = (fs::path(tempDir_r) / ("reducer_" + std::to_string(reducerId) + defaultLogFileSuffix)).string();
                        
                        Logger::getInstance().configureLogFilePath(logPath_r);
                        Logger::getInstance().setPrefix("[REDUCER_CMD_" + std::to_string(reducerId) + "] ");
                        Logger::getInstance().log("Starting Reducer Mode...");
                        
                        cmdModeSuccess = orchestrator.runReducer(outputDir_r, tempDir_r, reducerId, minPool_r, maxPool_r);
                        break;
                    }
                    default: 
                        currentMode = AppMode::INTERACTIVE; 
                        break; 
                }
            } catch (const std::invalid_argument& ia) {
                std::string err_msg = "MAIN_CMD_ARG_ERROR: Invalid argument for number conversion: " + std::string(ia.what());
                Logger::getInstance().log(err_msg); std::cerr << err_msg << std::endl;
                return EXIT_FAILURE;
            } catch (const std::out_of_range& oor) {
                 std::string err_msg = "MAIN_CMD_ARG_ERROR: Number out of range for conversion: " + std::string(oor.what());
                Logger::getInstance().log(err_msg); std::cerr << err_msg << std::endl;
                return EXIT_FAILURE;
            } catch (const std::exception& e) {
                std::string err_msg = "MAIN_CMD_ERROR: An unexpected error occurred: " + std::string(e.what());
                Logger::getInstance().log(err_msg); std::cerr << err_msg << std::endl;
                return EXIT_FAILURE;
            }

            if (currentMode != AppMode::INTERACTIVE) {
                 if (cmdModeSuccess) {
                    Logger::getInstance().log("MAIN_CMD: Mode completed successfully.");
                } else {
                    Logger::getInstance().log("MAIN_CMD: Mode reported failure.");
                }
                return cmdModeSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
            }
        }
    }

    if (currentMode == AppMode::INTERACTIVE) {
        // Logger for interactive mode is configured within runInteractiveWorkflow()
        return runInteractiveWorkflow(); 
    }
    
    return EXIT_SUCCESS; // Should be unreachable if logic is correct
}
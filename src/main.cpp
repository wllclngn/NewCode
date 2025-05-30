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

#include "..\include\ERROR_Handler.h"
#include "..\include\FileHandler.h"
#include "..\include\Logger.h"
#include "..\include\Mapper_DLL_so.h"
#include "..\include\Reducer_DLL_so.h"
#include "..\include\ProcessOrchestrator.h"
#include "..\include\InteractiveMode.h"

namespace fs = std::filesystem;

enum class AppMode {
    CONTROLLER,
    MAPPER,
    REDUCER,
    FINAL_REDUCER, // New mode for final reduction
    INTERACTIVE,
    UNKNOWN
};

// Helper function to signal reducers when mapper outputs are ready
void signalReducers(std::condition_variable &cv, std::mutex &mtx, bool &readyFlag) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        readyFlag = true;
    }
    cv.notify_all();
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

            try {
                switch (currentMode) {
                    case AppMode::CONTROLLER: {
                        // Base args: exec controller inputDir outputDir tempDir M R
                        if (argc < 7) {
                            std::cerr << "Controller usage: " << argv[0] << " controller <inputDir> <outputDir> <tempDir> <M> <R>" << std::endl;
                            return EXIT_FAILURE;
                        }
                        std::string inputDir = argv[2];
                        std::string outputDir = argv[3];
                        std::string tempDir = argv[4];
                        int numMappers = std::stoi(argv[5]);
                        int numReducers = std::stoi(argv[6]);

                        // Synchronization primitives for signaling reducers
                        std::condition_variable cvReducers;
                        std::mutex mtxReducers;
                        bool mapperOutputsReady = false;

                        // Launch mapper threads
                        std::vector<std::thread> mapperThreads;
                        for (int i = 0; i < numMappers; ++i) {
                            mapperThreads.emplace_back([&, i]() {
                                // Simulate mapper logic here
                                orchestrator.runMapper(tempDir, i, numReducers, {}, 2, 4);

                                if (i == numMappers - 1) { // Signal reducers when the last mapper finishes
                                    signalReducers(cvReducers, mtxReducers, mapperOutputsReady);
                                }
                            });
                        }

                        // Launch reducer threads concurrently
                        std::vector<std::thread> reducerThreads;
                        for (int i = 0; i < numReducers; ++i) {
                            reducerThreads.emplace_back([&, i]() {
                                // Wait until mapper outputs are ready
                                std::unique_lock<std::mutex> lock(mtxReducers);
                                cvReducers.wait(lock, [&]() { return mapperOutputsReady; });

                                // Simulate reducer logic here
                                orchestrator.runReducer(outputDir, tempDir, i, 2, 4);
                            });
                        }

                        // Wait for mappers to complete
                        for (auto &t : mapperThreads) {
                            t.join();
                        }

                        // Wait for reducers to complete
                        for (auto &t : reducerThreads) {
                            t.join();
                        }

                        // Perform final reduction
                        orchestrator.runFinalReducer(outputDir, tempDir);

                        // Write SUCCESS file
                        std::ofstream successFile(outputDir + "/SUCCESS.txt");
                        if (successFile.is_open()) {
                            successFile << "MapReduce job completed successfully.\n";
                            successFile.close();
                        } else {
                            Logger::getInstance().log("ERROR: Could not write SUCCESS file.", Logger::Level::ERROR);
                        }

                        cmdModeSuccess = true;
                        break;
                    }
                    // Other modes (e.g., MAPPER, REDUCER) handled here
                    default:
                        currentMode = AppMode::INTERACTIVE;
                        break;
                }
            } catch (const std::exception &e) {
                Logger::getInstance().log("MAIN_CMD_ERROR: " + std::string(e.what()), Logger::Level::ERROR);
                return EXIT_FAILURE;
            }

            return cmdModeSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    if (currentMode == AppMode::INTERACTIVE) {
        return runInteractiveWorkflow();
    }

    return EXIT_SUCCESS;
}
#pragma once
#include "logger.h"
#include "error_handler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <thread>

class Mapper {
public:
    void map_words(const std::vector<std::string>& lines, const std::string& tempFolderPath) {
        Logger::getInstance().log("Starting mapping phase...");

        std::string outputPath = tempFolderPath + "/mapped_temp.txt";
        std::ofstream temp_out(outputPath);
        if (!temp_out) {
            ErrorHandler::reportError("Failed to open " + outputPath + " for writing.", true);
            return;
        }

        size_t numThreads = std::thread::hardware_concurrency();
        size_t chunkSize = lines.size() / numThreads + (lines.size() % numThreads != 0);
        std::vector<std::thread> threads;
        std::mutex mutex;

        for (size_t i = 0; i < numThreads; ++i) {
            size_t startIdx = i * chunkSize;
            size_t endIdx = std::min(startIdx + chunkSize, lines.size());

            threads.emplace_back([this, &lines, startIdx, endIdx, &temp_out, &mutex]() {
                std::vector<std::pair<std::string, int>> localMapped;
                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        std::string cleanedWord = clean_word(word);
                        if (!cleanedWord.empty()) {
                            localMapped.emplace_back(cleanedWord, 1);
                        }
                    }
                }

                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localMapped) {
                    temp_out << "<" << kv.first << ", " << kv.second << ">" << std::endl;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        temp_out.close();
        Logger::getInstance().log("Mapping phase completed. Data written to " + outputPath);
    }

    static std::string clean_word(const std::string& word) {
        std::string result;
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                result += std::tolower(static_cast<unsigned char>(c));
            }
        }
        return result;
    }
};
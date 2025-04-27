#pragma once
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <mutex>
#include <thread>
#include <iostream>

class Mapper {
public:
    void map_words(const std::vector<std::string>& lines, const std::string& outputPath) {
        std::ofstream temp_out(outputPath);
        if (!temp_out) {
            std::cerr << "Error: Could not open " << outputPath << " for writing.\n";
            return;
        }

        std::mutex mutex;
        // ***** MAKE CHUNKING DYNAMIC ***** //
        size_t chunkSize = 1024;
        size_t numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, &lines, &temp_out, &mutex, i, chunkSize]() {
                size_t startIdx = i * chunkSize;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap;

                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        localMap[clean_word(word)]++;
                    }
                }

                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localMap) {
                    temp_out << kv.first << ": " << kv.second << "\n";
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        temp_out.close();
    }

private:
    std::string clean_word(const std::string& word) {
        std::string result;
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                result += std::tolower(static_cast<unsigned char>(c));
            }
        }
        return result;
    }
};
#pragma once
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
        std::string outputPath = tempFolderPath + "/mapped_temp.txt";
        std::ofstream temp_out(outputPath);
        if (!temp_out) {
            std::cerr << "Failed to open " << outputPath << " for writing." << std::endl;
            return;
        }

        std::cout << "Mapping words with cache-friendly chunking..." << std::endl;

        // Split the lines into cache-friendly chunks
        size_t numThreads = std::thread::hardware_concurrency();
        size_t chunkSize = 1024; // Approximate size for cache-friendliness (adjust based on system L1/L2 cache)
        std::vector<std::thread> threads;
        std::mutex mutex;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, &lines, i, chunkSize, &temp_out, &mutex]() {
                size_t startIdx = i * chunkSize;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());

                std::vector<std::pair<std::string, int>> localMapped;

                // Process lines in chunks
                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        std::string cleanedWord = clean_word(word);
                        if (!cleanedWord.empty()) {
                            localMapped.emplace_back(cleanedWord, 1);

                            // Write results to file when local chunk is full
                            if (localMapped.size() >= 100) {
                                std::lock_guard<std::mutex> lock(mutex);
                                for (const auto& kv : localMapped) {
                                    temp_out << "<" << kv.first << ", " << kv.second << ">" << std::endl;
                                }
                                localMapped.clear();
                            }
                        }
                    }
                }

                // Write any remaining results
                if (!localMapped.empty()) {
                    std::lock_guard<std::mutex> lock(mutex);
                    for (const auto& kv : localMapped) {
                        temp_out << "<" << kv.first << ", " << kv.second << ">" << std::endl;
                    }
                }
            });
        }

        // Join all threads
        for (auto& thread : threads) {
            thread.join();
        }

        temp_out.close();
        std::cout << "Mapping complete. Data written to " << outputPath << std::endl;
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
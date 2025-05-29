#ifndef MAPPER_DLL_SO_H
#define MAPPER_DLL_SO_H

#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <future>
#include <algorithm>

// Export macro for cross-platform compatibility
#if defined(_WIN32) || defined(_WIN64)
#define DLL_so_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define DLL_so_EXPORT __attribute__((visibility("default")))
#else
#define DLL_so_EXPORT
#endif

class DLL_so_EXPORT MapperDLLso {
public:
    virtual ~MapperDLLso() {} // Virtual destructor for polymorphism

    // Virtual function for character validation
    virtual bool is_valid_char(char c) const {
        return std::isalnum(static_cast<unsigned char>(c));
    }

    virtual std::string clean_word(const std::string &word) const {
        std::string result;

        // Iterate through each character in the word
        for (char c : word) {
            if (is_valid_char(c)) {
                result += std::tolower(static_cast<unsigned char>(c));
            }
        }

        // Allow words of any length that are not purely numeric
        if (result.empty() || std::all_of(result.begin(), result.end(), ::isdigit)) {
            return "";
        }

        return result;
    }

    virtual void map_words(const std::vector<std::string> &lines, const std::string &tempFilePath) {
        // Build out tempFilePath from user input for blank, "", user input directories.
        std::string fullDirPath;
        size_t last_slash_pos = tempFilePath.find_last_of('\\');

        // Check if a backslash was found
        if (last_slash_pos != std::string::npos) {
            // Dynamically build out tempFilePath for blank outPutFolder and tempFolder
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
        } else {
            last_slash_pos = tempFilePath.find_last_of('/');
            // Dynamically build out tempFilePath for blank outPutFolder and tempFolder
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
        }
        
        // Ensure the temporary folder path is valid
        if (!fs::exists(fullDirPath)) {
            report_error("Temporary folder does not exist: " + tempFilePath);
            return;
        }
        if (!fs::is_directory(fullDirPath)) {
            report_error("Temporary folder is not a directory: " + tempFilePath);
            return;
        }

        // Open the output file for writing
        std::ofstream temp_out(tempFilePath);
        if (!temp_out) {
            report_error("Could not open " + tempFilePath + " for writing.");
            return;
        }
      
        // Mutex for synchronizing file writes
        std::mutex file_mutex;

        // Calculate the optimal chunk size for processing
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());

        // Launch threads for parallel processing
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            futures.push_back(std::async(std::launch::async, [this, &lines, &temp_out, &file_mutex, i, chunkSize]() {
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap;

                // Perform local mapping
                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        localMap[clean_word(word)]++;
                    }
                }

                // Write the local map to the output file with synchronization
                {
                    std::lock_guard<std::mutex> lock(file_mutex);
                    for (const auto& kv : localMap) {
                        temp_out << kv.first << ": " << kv.second << "\n";
                    }
                }
            }));
        }

        // Wait for all threads to finish
        for (auto& future : futures) {
            future.get();
        }

        // Close the output file
        temp_out.close();
    }

protected:
    // Virtual function for reporting errors
    virtual void report_error(const std::string &error_message) const {
        std::cerr << "Error: " << error_message << std::endl;
    }

    size_t calculate_dynamic_chunk_size(size_t totalSize) const {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 1024;

        if (numThreads == 0) {
            return defaultChunkSize;
        }

        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }
};

#endif
#pragma once // Use #pragma once for modern compilers

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <sstream>
#include <algorithm>

#include "ERROR_Handler.h"
#include "Logger.h"

namespace fs = std::filesystem;

class FileHandler {
public:
    static bool read_mapped_data(const std::string &filename, std::vector<std::pair<std::string, int>> &mapped_data) {
        Logger::getInstance().log("FILE_HANDLER: Attempting to read mapped data from file: " + filename);

        std::ifstream infile(filename);
        if (!infile) {
            ErrorHandler::reportError("FILE_HANDLER: Could not open file " + filename + " for reading (mapped data).");
            return false;
        }

        std::string line;
        int line_number = 0;
        while (std::getline(infile, line)) {
            line_number++;

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos && colon_pos > 0) {
                std::string word = line.substr(0, colon_pos);
                std::string count_str = line.substr(colon_pos + 1);

                word = trim_string(word);
                count_str = trim_string(count_str);

                if (word.empty()) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Skipped line " + std::to_string(line_number) + " (empty word after trim): " + line);
                    continue;
                }
                if (count_str.empty()) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Skipped line " + std::to_string(line_number) + " (empty count string after trim): " + line);
                    continue;
                }

                try {
                    int count = std::stoi(count_str);
                    mapped_data.emplace_back(word, count);
                } catch (const std::invalid_argument& /*ia*/) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Invalid number format on line " + std::to_string(line_number) + " (count: '" + count_str + "'): " + line);
                } catch (const std::out_of_range& /*oor*/) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Number out of range on line " + std::to_string(line_number) + " (count: '" + count_str + "'): " + line);
                }
            } else {
                std::string trimmed_line = trim_string(line);
                if (!trimmed_line.empty()) {
                    Logger::getInstance().log("FILE_HANDLER: WARNING - Skipped malformed line " + std::to_string(line_number) + " (format error): " + line);
                }
            }
        }
        infile.close();

        if (mapped_data.empty() && line_number > 0) {
            Logger::getInstance().log("FILE_HANDLER: WARNING - No valid data parsed from file: " + filename + " (" + std::to_string(line_number) + " lines read).");
        } else {
            Logger::getInstance().log("FILE_HANDLER: Successfully read " + std::to_string(mapped_data.size()) + " entries from file: " + filename);
        }
        return true;
    }

    static std::string trim_string(const std::string& str) {
        std::string s = str;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
        return s;
    }
};
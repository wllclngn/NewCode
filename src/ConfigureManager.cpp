#include "..\include\ConfigManager.h"
#include "..\include\Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

bool ConfigManager::loadFromFile(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        Logger::getInstance().log("ConfigManager: Could not open configuration file: " + configFilePath);
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key-value pairs
        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) {
            Logger::getInstance().log("ConfigManager: Skipping invalid line: " + line);
            continue;
        }

        std::string key = trim(line.substr(0, delimiterPos));
        std::string value = trim(line.substr(delimiterPos + 1));

        if (!key.empty() && !value.empty()) {
            config[key] = value;
            Logger::getInstance().log("ConfigManager: Loaded configuration key '" + key + "' with value '" + value + "'");
        }
    }

    configFile.close();
    Logger::getInstance().log("ConfigManager: Configuration loaded successfully.");
    return true;
}

std::optional<size_t> ConfigManager::getMapperMinThreads() const {
    auto it = config.find("mapper_min_threads");
    return it != config.end() ? parseSizeT(it->second) : std::nullopt;
}

std::optional<size_t> ConfigManager::getMapperMaxThreads() const {
    auto it = config.find("mapper_max_threads");
    return it != config.end() ? parseSizeT(it->second) : std::nullopt;
}

std::optional<size_t> ConfigManager::getReducerMinThreads() const {
    auto it = config.find("reducer_min_threads");
    return it != config.end() ? parseSizeT(it->second) : std::nullopt;
}

std::optional<size_t> ConfigManager::getReducerMaxThreads() const {
    auto it = config.find("reducer_max_threads");
    return it != config.end() ? parseSizeT(it->second) : std::nullopt;
}

std::string ConfigManager::getIntermediateFileFormat() const {
    auto it = config.find("intermediate_file_format");
    return it != config.end() ? it->second : "temp/partition_{mapper_id}_{reducer_id}.txt";
}

std::string ConfigManager::getOutputFileFormat() const {
    auto it = config.find("output_file_format");
    return it != config.end() ? it->second : "output/reducer_{reducer_id}.txt";
}

void ConfigManager::setMapperMinThreads(size_t minThreads) {
    config["mapper_min_threads"] = std::to_string(minThreads);
}

void ConfigManager::setMapperMaxThreads(size_t maxThreads) {
    config["mapper_max_threads"] = std::to_string(maxThreads);
}

void ConfigManager::setReducerMinThreads(size_t minThreads) {
    config["reducer_min_threads"] = std::to_string(minThreads);
}

void ConfigManager::setReducerMaxThreads(size_t maxThreads) {
    config["reducer_max_threads"] = std::to_string(maxThreads);
}

void ConfigManager::setIntermediateFileFormat(const std::string& format) {
    config["intermediate_file_format"] = format;
}

void ConfigManager::setOutputFileFormat(const std::string& format) {
    config["output_file_format"] = format;
}

// Helper to trim whitespace from a string
std::string ConfigManager::trim(const std::string& str) {
    const auto strBegin = str.find_first_not_of(" \t");
    if (strBegin == std::string::npos) {
        return ""; // No content
    }

    const auto strEnd = str.find_last_not_of(" \t");
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

// Helper to parse size_t safely
std::optional<size_t> ConfigManager::parseSizeT(const std::string& value) {
    try {
        size_t result = std::stoul(value);
        return result;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
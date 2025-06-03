#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <ctime>   // For std::time_t, std::tm
#include <iomanip> // For std::put_time
#include <sstream> // For std::ostringstream for timestamp

class Logger {
public:
    enum class Level {
        INFO,
        DEBUG,
        ERROR,
        WARNING
    };

    static Logger& getInstance() {
        static Logger instance; // Meyers' Singleton
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void configureLogFilePath(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFilePath_ = path; 
        logFile_.open(path, std::ios::app); 
        if (!logFile_) {
            std::cerr << "[LOGGER_ERROR] Could not open log file for writing: " << path << std::endl;
        } else {
            // Removed cout here as logger.log below will also print to cout by default
            // std::cout << "[LOGGER_INFO] Log file configured at: " << path << std::endl;
        }
    }

    void setPrefix(const std::string& prefix) {
        std::lock_guard<std::mutex> lock(mutex_);
        logPrefix_ = prefix;
    }

    void log(const std::string& message, Level level = Level::INFO) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string levelStr = getLevelString(level);
        // Use public getTimestamp() for formatting
        std::string fullMessage = "[" + getTimestamp() + "] [" + levelStr + "] " + logPrefix_ + message;
        
        if (logFile_.is_open()) {
            logFile_ << fullMessage << std::endl;
        } else {
            std::cerr << "[LOG_TO_CERR] " << fullMessage << std::endl;
        }
        std::cout << fullMessage << std::endl;
    }

    // Made public for use in main.cpp SUCCESS file. Consider alternatives for better encapsulation.
    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        std::tm timeinfo{}; // Value-initialize
        #ifdef _WIN32
            localtime_s(&timeinfo, &in_time_t);
        #else
            localtime_r(&in_time_t, &timeinfo); // POSIX version
        #endif
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    Logger() {} 
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    std::string getLevelString(Level level) { // Keep private if only used by log()
        switch (level) {
            case Level::INFO: return "INFO";
            case Level::DEBUG: return "DEBUG";
            case Level::ERROR: return "ERROR";
            case Level::WARNING: return "WARNING";
            default: return "UNKNOWN";
        }
    }

    std::ofstream logFile_;
    std::string logFilePath_;
    std::string logPrefix_;  
    std::mutex mutex_;       
};
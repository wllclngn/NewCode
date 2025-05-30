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
    static Logger& getInstance() {
        static Logger instance; // Meyers' Singleton
        return instance;
    }

    // Deleted copy constructor and assignment operator for Singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void configureLogFilePath(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFilePath_ = path; // Store for potential re-opening or reference
        logFile_.open(path, std::ios::app); // Open in append mode
        if (!logFile_) {
            std::cerr << "[LOGGER_ERROR] Could not open log file for writing: " << path << std::endl;
        } else {
            // Optionally log that the log file was configured
            // log("Log file configured at: " + path); // careful with re-entrancy if called from here
            std::cout << "[LOGGER_INFO] Log file configured at: " << path << std::endl;
        }
    }

    void setPrefix(const std::string& prefix) {
        std::lock_guard<std::mutex> lock(mutex_);
        logPrefix_ = prefix;
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string fullMessage = "[" + getTimestamp() + "] " + logPrefix_ + message;
        
        if (logFile_.is_open()) {
            logFile_ << fullMessage << std::endl;
        } else {
            // Fallback to cerr if log file isn't open, but indicate it's a log message
            std::cerr << "[LOG_TO_CERR] " << fullMessage << std::endl;
        }
        // Always log to cout as per original behavior
        std::cout << fullMessage << std::endl;
    }

private:
    Logger() {} // Private constructor for Singleton
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    std::string getTimestamp() {
        // Using std::ostringstream for cleaner timestamp formatting
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        // For MSVC, std::localtime might require _s version.
        // std::tm buf;
        // localtime_s(&buf, &in_time_t);
        // ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
        // For standard C++ (POSIX, MinGW):
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    std::ofstream logFile_;
    std::string logFilePath_; // Store the path
    std::string logPrefix_;   // Store a prefix for log messages
    std::mutex mutex_;        // Mutex to protect file access and prefix changes
};

#endif // LOGGER_H (guard typically not needed for #pragma once but good practice)
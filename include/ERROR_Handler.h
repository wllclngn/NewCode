#pragma once
#include <string>
#include <iostream>

class ErrorHandler {
public:
    // Updated to accept two arguments: errorMessage (string) and critical (bool)
    static void reportError(const std::string& errorMessage, bool critical = false) {
        std::cerr << "[ERROR] " << errorMessage << std::endl;
        if (critical) {
            std::cerr << "Critical error encountered. Exiting program." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Handles missing identifiers gracefully
    static void handleMissingIdentifier(const std::string& identifier) {
        std::cerr << "[ERROR] Missing or undeclared identifier: " << identifier << std::endl;
    }

    // Handles invalid types or symbols
    static void handleInvalidTypeOrSymbol(const std::string& symbol) {
        std::cerr << "[ERROR] Invalid type or symbol: " << symbol << std::endl;
    }

    // Logs a general warning
    static void logWarning(const std::string& warningMessage) {
        std::cerr << "[WARNING] " << warningMessage << std::endl;
    }
};
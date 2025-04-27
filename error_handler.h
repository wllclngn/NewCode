#pragma once
#include <string>
#include <iostream>

class ErrorHandler {
public:
    static void reportError(const std::string& errorMessage, bool critical = false) {
        std::cerr << "[ERROR] " << errorMessage << std::endl;
        if (critical) {
            std::cerr << "Critical error encountered. Exiting program." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};
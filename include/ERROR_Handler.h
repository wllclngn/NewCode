#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <iostream>

class ErrorHandler {
public:
    // Singleton instance getter
    static ErrorHandler& getInstance() {
        static ErrorHandler instance;
        return instance;
    }

    // Report an error
    void reportError(const std::string& errorMessage) {
        std::cerr << "ERROR: " << errorMessage << std::endl;
    }

private:
    // Private constructor to prevent instantiation
    ErrorHandler() = default;
    ~ErrorHandler() = default;

    // Delete copy constructor and assignment operator
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
};

#endif // ERROR_HANDLER_H
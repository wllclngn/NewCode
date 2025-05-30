#ifndef MAPPER_DLL_SO_H
#define MAPPER_DLL_SO_H

#include <string>
#include <vector>
#include <utility>

class Logger;
class ErrorHandler;

#if defined(_WIN32) || defined(_WIN64)
    #ifdef DLL_so_EXPORTS
        #define DLL_so_EXPORT __declspec(dllexport)
    #else
        #define DLL_so_EXPORT __declspec(dllimport)
    #endif
#elif (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
    #ifdef DLL_so_EXPORTS
        #define DLL_so_EXPORT __attribute__((visibility("default")))
    #else
        #define DLL_so_EXPORT
    #endif
#else
    #define DLL_so_EXPORT
#endif


class DLL_so_EXPORT Mapper {
public:

    Mapper(Logger& logger, ErrorHandler& errorHandler);

    ~Mapper();

    void map(const std::string& documentId, const std::string& line, std::vector<std::pair<std::string, int>>& intermediateData);

    bool exportMappedData(const std::string& filePath, const std::vector<std::pair<std::string, int>>& mappedData);

private:
    Logger& logger; 
    ErrorHandler& errorHandler;
};

#endif // MAPPER_DLL_SO_H
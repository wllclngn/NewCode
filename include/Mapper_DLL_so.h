#ifndef MAPPER_DLL_SO_H
#define MAPPER_DLL_SO_H

#include "ExportDefinitions.h"
#include <string>
#include <vector>
#include <utility>

class Logger;
class ErrorHandler;

class DLL_so_EXPORT Mapper {
public:
    Mapper(Logger& logger, ErrorHandler& errorHandler);
    ~Mapper();

    void map(const std::string& documentId, const std::string& line, std::vector<std::pair<std::string, int>>& intermediateData);

    bool exportPartitionedData(const std::string& tempDir, const std::vector<std::pair<std::string, int>>& mappedData, int numReducers);

    bool exportMappedData(const std::string& filePath, const std::vector<std::pair<std::string, int>>& mappedData);

private:
    Logger& logger;
    ErrorHandler& errorHandler;
};

#endif // MAPPER_DLL_SO_H
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <unordered_map>
#include <optional>

class ConfigManager {
public:
    // Load configuration from a JSON-like file (manual parsing)
    bool loadFromFile(const std::string& configFilePath);

    // Get thread pool configuration
    std::optional<size_t> getMapperMinThreads() const;
    std::optional<size_t> getMapperMaxThreads() const;
    std::optional<size_t> getReducerMinThreads() const;
    std::optional<size_t> getReducerMaxThreads() const;

    // Get file naming conventions
    std::string getIntermediateFileFormat() const;
    std::string getOutputFileFormat() const;

    // Set configuration overrides
    void setMapperMinThreads(size_t minThreads);
    void setMapperMaxThreads(size_t maxThreads);
    void setReducerMinThreads(size_t minThreads);
    void setReducerMaxThreads(size_t maxThreads);
    void setIntermediateFileFormat(const std::string& format);
    void setOutputFileFormat(const std::string& format);

private:
    std::unordered_map<std::string, std::string> config;

    // Helper to trim whitespace from a string
    static std::string trim(const std::string& str);

    // Helper to parse size_t safely
    static std::optional<size_t> parseSizeT(const std::string& value);
};

#endif // CONFIG_MANAGER_H
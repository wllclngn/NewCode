#ifndef MAPPER_DLL_SO_H
#define MAPPER_DLL_SO_H

#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

// Export macro for cross-platform compatibility
#if defined(_WIN32) || defined(_WIN64)
#define DLL_so_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define DLL_so_EXPORT __attribute__((visibility("default")))
#else
#define DLL_so_EXPORT
#endif

class DLL_so_EXPORT Mapper {
public:
    static bool is_valid_char(char c);
    static std::string clean_word(const std::string &word);
    void map_words(const std::vector<std::string> &lines, const std::string &tempFolderPath);

private:
    std::vector<std::pair<std::string, int>> mapped;
    void write_chunk_to_file(std::ofstream &outfile);
};

#endif
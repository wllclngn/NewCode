#ifndef MAPPER_DLL_H
#define MAPPER_DLL_H

#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

// Export macro for Windows DLLs
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

class DLL_EXPORT Mapper
{
public:
    static bool is_valid_char(char c);
    static std::string clean_word(const std::string &word);

    void map_words(const std::vector<std::string> &lines, const std::string &tempFolderPath);

private:
    std::vector<std::pair<std::string, int>> mapped;
    void write_chunk_to_file(std::ofstream &outfile);
};

#endif // MAPPER_DLL_H
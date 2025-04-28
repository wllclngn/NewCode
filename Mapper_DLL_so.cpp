#ifndef MAPPER_DLL_so_H
#define MAPPER_DLL_so_H

#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

// Export macro for Windows DLL_sos and Linux .so
#if defined(_WIN32) || defined(_WIN64)
#define DLL_so_EXPORT __declspec(DLL_soexport)
#elif defined(__linux__) || defined(__unix__)
#define DLL_so_EXPORT __attribute__((visibility("default")))
#else
#define DLL_so_EXPORT
#endif

class DLL_so_EXPORT Mapper
{
public:
    static bool is_valid_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c));
    }

    static std::string clean_word(const std::string &word)
    {
        std::string result;
        for (char c : word)
        {
            if (is_valid_char(c))
            {
                result += std::tolower(static_cast<unsigned char>(c));
            }
        }
        return result;
    }

    void map_words(const std::vector<std::string> &lines, const std::string &tempFolderPath)
    {
        // Ensure cross-platform path handling
        #ifdef _WIN32
        std::string outputPath = tempFolderPath + "\\mapped_temp.txt";
        #else
        std::string outputPath = tempFolderPath + "/mapped_temp.txt";
        #endif

        std::ofstream temp_out(outputPath);
        if (!temp_out)
        {
            std::cerr << "Failed to open " << outputPath << " for writing." << std::endl;
            return;
        }

        std::cout << "Mapping words..." << std::endl;
        for (const auto &line : lines)
        {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word)
            {
                std::string cleaned = clean_word(word);
                if (!cleaned.empty())
                {
                    mapped.push_back({cleaned, 1});
                    if (mapped.size() >= 100)
                    {
                        write_chunk_to_file(temp_out);
                        mapped.clear();
                    }
                }
            }
        }

        if (!mapped.empty())
        {
            write_chunk_to_file(temp_out);
            mapped.clear();
        }

        temp_out.close();
        std::cout << "Mapping complete. Data written to " << outputPath << std::endl;
    }

private:
    std::vector<std::pair<std::string, int>> mapped;

    void write_chunk_to_file(std::ofstream &outfile)
    {
        for (const auto &kv : mapped)
        {
            outfile << "<" << kv.first << ", " << kv.second << ">" << std::endl;
        }
    }
};

#endif
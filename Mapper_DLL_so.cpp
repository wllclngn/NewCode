#include "Mapper_DLL_so.h"

bool Mapper::is_valid_char(char c)
{
    return std::isalnum(static_cast<unsigned char>(c));
}

std::string Mapper::clean_word(const std::string &word)
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

void Mapper::map_words(const std::vector<std::string> &lines, const std::string &tempFolderPath)
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

void Mapper::write_chunk_to_file(std::ofstream &outfile)
{
    for (const auto &kv : mapped)
    {
        outfile << "<" << kv.first << ", " << kv.second << ">" << std::endl;
    }
}
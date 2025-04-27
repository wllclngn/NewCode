#pragma once
#include <string>
#include <filesystem>

namespace Utils {
    bool is_valid_directory(const std::string& path) {
        return std::filesystem::exists(path) && std::filesystem::is_directory(path);
    }

    void remove_trailing_slash(std::string& path) {
        if (!path.empty() && (path.back() == '/' || path.back() == '\\')) {
            path.pop_back();
        }
    }
}
#include <iostream>
#include <chrono>
#include "mapper.h"
#include "reducer.h"

int main() {
    // Example input data
    std::vector<std::string> lines = {
        "This is a test",
        "Another test line",
        "And yet another line of text"
    };

    Mapper mapper;
    Reducer reducer;

    // Measure mapping time
    auto start = std::chrono::high_resolution_clock::now();
    mapper.map_words(lines, "./temp");
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Mapping Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";

    // Simulate mapped data for reducing
    std::vector<std::pair<std::string, int>> mappedData = {
        {"this", 1}, {"is", 1}, {"a", 1}, {"test", 1},
        {"another", 1}, {"test", 1}, {"line", 1},
        {"and", 1}, {"yet", 1}, {"another", 1}, {"line", 1}, {"of", 1}, {"text", 1}
    };

    // Measure reducing time
    start = std::chrono::high_resolution_clock::now();
    reducer.reduce(mappedData);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Reducing Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";

    return 0;
}

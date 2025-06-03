#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <string>
#include <functional>

class Partitioner {
public:
    // Constructor to initialize with the number of reducers
    explicit Partitioner(int numReducers) : numReducers(numReducers) {}

    // Function to determine the reducer bucket for a given key
    int getReducerBucket(const std::string& key) const {
        std::hash<std::string> hashFn;
        return hashFn(key) % numReducers;
    }

private:
    int numReducers; // Number of reducers
};

#endif // PARTITIONER_H
#pragma once
#include <iostream>
#include <string>

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::cerr << "[FAIL] " << __FUNCTION__ << "(): " << __LINE__ << ": Expected '" << (expected) << "', but got '" << (actual) << "'.\n"; \
    } else { \
        std::cout << "[PASS] " << __FUNCTION__ << "(): " << __LINE__ << "\n"; \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "[FAIL] " << __FUNCTION__ << "(): " << __LINE__ << ": Condition '" << #condition << "' is false.\n"; \
    } else { \
        std::cout << "[PASS] " << __FUNCTION__ << "(): " << __LINE__ << "\n"; \
    }

#define TEST_CASE(name) \
    void name(); \
    int main() { \
        name(); \
        return 0; \
    } \
    void name()
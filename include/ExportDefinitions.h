#ifndef EXPORT_DEFINITIONS_H
#define EXPORT_DEFINITIONS_H

// Export macro for cross-platform compatibility
#if defined(_WIN32) || defined(_WIN64)
    #define DLL_so_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    #define DLL_so_EXPORT __attribute__((visibility("default")))
#else
    #define DLL_so_EXPORT
#endif

#endif // EXPORT_DEFINITIONS_H
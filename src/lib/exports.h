// src/lib/exports.h
#ifndef CUSTOMDB_EXPORTS_H
#define CUSTOMDB_EXPORTS_H

// Определяем, собираем ли мы библиотеку или используем её
#ifdef _WIN32
    #ifdef CUSTOMDB_BUILD_DLL
        #define CUSTOMDB_API __declspec(dllexport)
    #else
        #define CUSTOMDB_API __declspec(dllimport)
    #endif
#else
    // Для Linux/macOS
    #ifdef CUSTOMDB_BUILD_DLL
        #define CUSTOMDB_API __attribute__((visibility("default")))
    #else
        #define CUSTOMDB_API
    #endif
#endif

#endif // CUSTOMDB_EXPORTS_H
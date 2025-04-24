#pragma once

#include <stdio.h>

enum logging_level {
    LOG_ERROR = 1,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
};

#if (!CONFIG_LOG)

typedef struct {
    const char name[16];
    enum logging_level level;
}logging_t;

#define LOG_MODULE_REGISTER(module_name, module_level) \
    static logging_t __log_module = { \
        .name = #module_name, \
        .level = module_level, \
    }

#define LOG_ERR(...) \
    do { \
        if (__log_module.level >= LOG_ERROR) { \
            printf("<ERR> %s: ", __log_module.name); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

#define LOG_WRN(...) \
    do { \
        if (__log_module.level >= LOG_WARNING) { \
            printf("<WRN> %s: ", __log_module.name); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

#define LOG_INF(...) \
    do { \
        if (__log_module.level >= LOG_INFO) { \
            printf("<INF> %s: ", __log_module.name); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

#define LOG_DBG(...) \
    do { \
        if (__log_module.level >= LOG_DEBUG) { \
            printf("<DBG> %s: ", __log_module.name); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

#endif
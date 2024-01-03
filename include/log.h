#ifndef LOG_H
#define LOG_H
#include <stdlib.h>
#include <stdint.h>

//log levels
enum levels {
    ERROR,
    WARN,
    INFO,
    DEBUG
};

uint8_t get_level(enum levels level);

#define LOG(level, message) \
    printf("[%c][%s:%3d]: %s\n", get_level(level), __FILE__, __LINE__, message);

//seperate them until I can make NULL va args work
#define LOGF(level, format, ...) \
    printf("[%c][%s:%3d]: " format "\n", get_level(level), __FILE__, __LINE__, __VA_ARGS__);
    

#endif

#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <cstdarg>
#include <Windows.h>

class Logger {
public:
    Logger(const std::string& directory);
    ~Logger();

    void Log(const char* format, ...);

private:
    std::ofstream logFile;
    std::string GetTimestamp();
    std::string FormatString(const char* format, va_list args);
};

extern Logger logger;

#define LOG(...) logger.Log(__VA_ARGS__)
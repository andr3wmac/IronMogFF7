#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <cstdarg>

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

#if _DEBUG
#define DEBUG_LOG(...) logger.Log(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif
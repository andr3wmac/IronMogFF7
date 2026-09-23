#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <cstdarg>
#include <mutex>

class Logger {
public:
    Logger(const std::string& directory);
    ~Logger();

    void Log(const char* format, ...);

private:
    std::ofstream logFile;
    std::mutex logMutex; // LOG is called from both the GUI and game manager threads.
    std::string getTimestamp();
    std::string formatString(const char* format, va_list args);
};

extern Logger logger;

#define LOG(...) logger.Log(__VA_ARGS__)

#if _DEBUG
#define DEBUG_LOG(...) logger.Log(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif
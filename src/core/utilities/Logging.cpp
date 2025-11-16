#include "Logging.h"
#include <filesystem>
#include <iomanip>
#include <sstream>

#define NOMINMAX
#include <windows.h>

Logger logger("logs");

Logger::Logger(const std::string& directory) 
{
    std::filesystem::create_directories(directory);

    // Generate timestamped filename
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);

    std::ostringstream filename;
    filename << directory << "/log_" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".txt";
    logFile.open(filename.str(), std::ios::out | std::ios::app);
}

Logger::~Logger() 
{
    if (logFile.is_open()) 
    {
        logFile.close();
    }
}

void Logger::Log(const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    std::string formatted = formatString(format, args);
    va_end(args);

    std::string line = getTimestamp() + " " + formatted + "\n";

    // Write to file
    if (logFile.is_open()) 
    {
        logFile << line;
        logFile.flush();
    }

    // Write to Visual Studio debug console
    OutputDebugStringA(line.c_str());
}

std::string Logger::formatString(const char* format, va_list args) 
{
    char buffer[1024];
    vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    return std::string(buffer);
}

std::string Logger::getTimestamp() 
{
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "[%H:%M:%S]", &tm);
    return std::string(buffer);
}

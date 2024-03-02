#pragma once

#ifndef __ATF_USER_LOGGING__
#define __ATF_USER_LOGGING__

#if defined(_WIN32)
#include <Windows.h>
#endif //_WIN32

#include <string>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>

//
// Examples:
//  LOG_DEBUG("Starting");
//  Logger::GetInstance() << "test" << abc;
//  LOG_PRINTF("Test %s", "asdfasdf");
//

class Logger;

//
// Log on the default log source
//
#define LOG_DEBUG(x) Logger::GetInstance().Log(x)
#define LOG_PRINTF(format, ...) Logger::GetInstance().Log(format, ##__VA_ARGS__)

//
// Input: moduleName is the name of the executable, dll, whichever
// Input: logType indicates where to log
//
#define LOG_INIT(modName, logType) Logger::Initialize(modName, logType)

//
// This Logger class can be implemented header only, all instances are static and global for ease and comfort or something
//
// TODO: Implement an interface
//

static Logger *currentInstance;
static std::mutex logSync;

class Logger {
public:
    typedef enum _logging_type {
        _logging_type_file,
        _logging_type_windows_debug,
        _logging_type_console,
        _logging_type_all
    } LOGGING_TYPE;

private:
    const std::string moduleName;
    const std::string loggingFile;
    const LOGGING_TYPE loggingType;


    std::mutex fileSync;

private:
    Logger(const std::string &moduleName, enum _logging_type loggingType) :
        moduleName(moduleName),
        loggingType(loggingType)
    {
        
    }

    Logger(const std::string &moduleName, enum _logging_type loggingType, const std::string &loggingFile) :
        moduleName(moduleName),
        loggingFile(loggingFile),
        loggingType(loggingType)
    {
        
    }

    ~Logger(void)
    {
        
    }

    std::string GenerateOutputString(const std::string &s)
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        std::tm now_tm;
        localtime_s(&now_tm, &now_c);

        std::stringstream ss;
        ss << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] [" << moduleName << "] " << s << std::endl;

        return ss.str();
    }

    void LogInst(enum _logging_type type, const std::string &s)
    {
        switch (type) {
        case _logging_type_windows_debug:
        
#if defined (_WIN32)       
            OutputDebugStringA(GenerateOutputString(s).c_str());
#endif //_WIN32
        
            break;

        case _logging_type_file:
            break;
        case _logging_type_console:
            std::cout << "[" << moduleName << "] " << s << std::endl;
            break;
        case _logging_type_all:
            break;
        default:
            break;
        }
    }

    // TODO -- fix this crap
    template<typename... Args>
    std::string formatString(const std::string& format, Args... args) {
        const int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
        if (size_s <= 0) { 
            return "";
        }
        auto size = static_cast<size_t>(size_s);
        auto buf = std::make_unique<char[]>(size);
        std::snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

public:
    static void Initialize(const std::string &moduleName, enum _logging_type loggingType)
    {
        std::lock_guard<std::mutex> lock(logSync);

        if (!currentInstance) {
            currentInstance = new Logger(moduleName, loggingType);
        }
    }

    static inline Logger &GetInstance(void)
    {
        return *currentInstance;
    }

    static Logger DestroyLogger(void)
    {
        std::lock_guard<std::mutex> lock(logSync);
        if (currentInstance) {
            delete currentInstance;
            currentInstance = nullptr;
        }
    }

    template<typename T>
    Logger &operator<<(const T &s) {
        std::ostringstream os;
        os << s;
        Log(os.str());
        return *this;
    }

    static inline void Log(enum _logging_type type, const std::string &s)
    {
        std::lock_guard<std::mutex> lock(logSync);
        if (!currentInstance) {
            return;
        }

        currentInstance->LogInst(type, s);
    }

    template<typename... Args>
    void Log(const std::string& format, Args... args) {
        std::string formattedMessage = formatString(format, args...);
        Log(formattedMessage);
    }

    static inline void Log(const std::string &s)
    {
        std::lock_guard<std::mutex> lock(logSync);
        if (!currentInstance) {
            return;
        }

        currentInstance->LogInst(currentInstance->loggingType, s);
    }
};

#endif //__ATF_USER_LOGGING__
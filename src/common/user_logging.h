#pragma once

#ifndef __ATF_USER_LOGGING__
#define __ATF_USER_LOGGING__

#include <string>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>

class Logger;
#define LOG_DEBUG(x) Logger::GetInstance()->Log(x)
#define LOG_INIT(modName, logType) Logger::Initialize(modName, logType)

//
// This Logger class can be implemented header only, all instances are static and global for ease and comfort or something
//
class Logger {
public:
    typedef enum _logging_type {
        _logging_type_file,
        _logging_type_windows_debug,
        _logging_type_console,
        _logging_type_all
    } LOGGING_TYPE;

private:
    static Logger *currentInstance;
    static std::mutex logSync;

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
        ss << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] [" << moduleName << "] " << s;

        return ss.str();
    }

    void LogInst(enum _logging_type type, const std::string &s)
    {
        switch (type) {
        case _logging_type_windows_debug:
        {
#if defined (_WIN32)       
            OutputDebugStringA(Logger::GenerateOutputString(s).c_str());
#endif //_WIN32
        }
        break;

        case _logging_type_file:

        case _logging_type_console:
            std::cout << "[" << moduleName << "] " << s << std::endl;
            break;
        case _logging_type_all:
        }
    }

public:
    static void Initialize(const std::string &moduleName, enum _logging_type loggingType)
    {
        std::lock_guard<std::mutex> lock(logSync);

        if (!Logger::currentInstance) {
            Logger::currentInstance = new Logger(moduleName, loggingType);
        }
    }

    static Logger *GetInstance(void) 
    {
        return currentInstance;
    }

    static Logger DestroyLogger(void)
    {
        std::lock_guard<std::mutex> lock(logSync);
        if (Logger::currentInstance) {
            delete Logger::currentInstance;
            Logger::currentInstance = nullptr;
        }
    }

    static void Log(enum _logging_type type, const std::string &s)
    {
        std::lock_guard<std::mutex> lock(logSync);
        if (!Logger::currentInstance) {
            return;
        }

        Logger::currentInstance->LogInst(type, s);
    }

    static void Log(const std::string &s)
    {
        std::lock_guard<std::mutex> lock(logSync);
        if (!Logger::currentInstance) {
            return;
        }

        Logger::currentInstance->LogInst(currentInstance->loggingType, s);
    }
};

#endif //__ATF_USER_LOGGING__
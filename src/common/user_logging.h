#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#if defined(_WIN32)
#include <Windows.h>
#endif //_WIN32

#include <string>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <map>
#include <vector>
#include <stdint.h>

//
// Notes:
//  No need to destroy or free the Logger, it will self-destroy at end of scope
//  Only once instance of the Logger will exist, regardless of where the header is imported
//
// Examples:
// 
// Initialization:
//  LOG_INIT(DRIVER_CTL_NAME, LOG_SOURCE_FILE); 
//  LOG_INIT(DRIVER_CTL_NAME, { LOG_SOURCE_WINDOWS_DEBUG, LOG_SOURCE_CONSOLE });
// 
// Log to the default source, log as the default alert level (INFO)
//  LOG_DEFAULT("Starting");
//  Logger::GetInstance() << "test" << abc;
//  LOG_PRINT("Test %s", "asdfasdf");
//  LOG_DEFAULT(test3 + DRIVER_CTL_NAME + "asdfasdf");
// 
// Specify log severity/level:
//  LOG_ERROR("Severe error: 0x%08x", res);
// 
// Use the fast logger:
//  LOG_FAST
//

// Forward declare
class Logger;

//
// Input: moduleName is the name of the executable, dll, whichever
// Input: logType indicates where to log
//
#define LOG_INIT(modName, logType)          Logger::Initialize(modName, logType)

//
// Log sources
//
#define LOG_SOURCE_FILE                     Logger::_logging_type_dest::_logging_type_dest_file
#define LOG_SOURCE_WINDOWS_DEBUG            Logger::_logging_type_dest::_logging_type_dest_windows_debug
#define LOG_SOURCE_CONSOLE                  Logger::_logging_type_dest::_logging_type_dest_console
#define LOG_SOURCE_ALL                      Logger::_logging_type_dest::_logging_type_dest_all

//
// Log on the default log destination, default alert level (info)
//
#define LOG_DEFAULT(x)                      Logger::GetInstance().Log(x)

//
// Log on the default log destination, specify level
//
#define LOG_DEBUG(format, ...)              Logger::GetInstance().Logf(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define LOG_SILENT(format, ...)             Logger::GetInstance().Logf(LOG_LEVEL_SILENT, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)              Logger::GetInstance().Logf(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)            Logger::GetInstance().Logf(LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)               Logger::GetInstance().Logf(LOG_LEVEL_INFO, format, ##__VA_ARGS__)

//
// Fast logger that does not use variadic functions
//
#define LOG_FAST(x, y)                      Logger::GetInstance().Log(x, y)
#define LOG(x)                              Logger::GetInstance().Log(x)


//
// This Logger class can be implemented header only, all instances are static and global for ease and comfort or something
//
// TODO: Implement an interface
//

//
// Log levels
//
#define LOG_LEVEL_SILENT                    0
#define LOG_LEVEL_ERROR                     1
#define LOG_LEVEL_WARNING                   2
#define LOG_LEVEL_INFO                      3
#define LOG_LEVEL_DEBUG                     4
#define LOG_LEVEL_DEFAULT_TYPE              LOG_LEVEL_INFO


//
// Synchronize user calls
//
static std::mutex logSync;

class Logger {

    //
    // Current instance of the logger, only one exists after call to Initialize()
    //
    static inline std::unique_ptr<Logger>   currentInstance;


public:
    typedef uint8_t LogLevel;

    typedef enum _logging_type_dest {
        _logging_type_dest_file,
        _logging_type_dest_windows_debug,
        _logging_type_dest_console,
        _logging_type_dest_all
    } LOGGING_TYPE;

private:
    static const LogLevel defaultLogLevel = LOG_LEVEL_DEFAULT_TYPE;  
    const std::map<LogLevel, std::string> logLevelDesc = {
        { LOG_LEVEL_SILENT,                 "SILENT" },
        { LOG_LEVEL_ERROR,                  "ERROR" },
        { LOG_LEVEL_WARNING,                "WARNING" },
        { LOG_LEVEL_INFO,                   "INFO" },
    };

private:
    // Name of the module, for example the DLL or executable, the instance of the Logger
    const std::string moduleName;

    // Specify the filename for logging
    const std::string loggingFileName;

    // Specify the logging destination
    const std::vector<enum _logging_type_dest> loggingTypeDest;


    std::mutex fileSync;    

public:
    //
    // Initialze very basic logging type
    //
    Logger(const std::string &moduleName, 
        const std::vector<enum _logging_type_dest> &loggingTypeDest) :

        moduleName(moduleName),
        loggingTypeDest({loggingTypeDest}),
        loggingFileName("")
    {

    }


    //
    // Initialze, but specify the logging output file
    //
    Logger(const std::string &moduleName, 
        const std::vector<enum _logging_type_dest> &loggingTypeDest, 
        const std::string &loggingFile) :

        moduleName(moduleName),
        loggingFileName(loggingFile),
        loggingTypeDest({loggingTypeDest})
    {

    }

    ~Logger(void)
    {

    }

private:
    static inline void generateDatetimeString(std::ostringstream &ss);

    // Note, cannot be static because logLevelDesc cannot be statically initialized
    inline void generateAlertLevelString(LogLevel level, std::ostringstream &ss) const;

    // Add PID
    static inline void generatePid(std::ostringstream &ss);

    // Primary logging function
    inline void LogInst(LogLevel level, const std::vector<enum _logging_type_dest> &type, const std::string &s);


    // TODO -- fix this crap
    template<typename... Args>
    static std::string formatString(const std::string& format, Args... args);

public:
    //
    // Initialize the Logger instance, will be automatically free'd end of scope
    //
    static inline void Initialize(const std::string &moduleName, std::vector<enum _logging_type_dest> types);    
    static inline void Initialize(const std::string &moduleName, enum _logging_type_dest loggingType);

    static inline Logger &GetInstance(void);

    // Allow << logging, for example `Logger::GetInstance() << "log data"`;
    template<typename T>
    Logger &operator<<(const T &s);

    // Wrappers for LogInst()
    template<typename... Args>
    static inline void Logf(const std::string& format, Args... args);

    template<typename... Args>
    inline void Logf(LogLevel level, const std::string& format, Args... args);
    
    static inline void Log(LogLevel level, enum _logging_type_dest type, const std::string &s);

    static inline void Log(LogLevel level, const char *s);

    template<typename T>
    static inline void Log(LogLevel level, const T &s);

    // Default log type overrides
    static inline void Log(const char *s);

    template<typename T>
    static inline void Log(const T &s);
};

inline void Logger::generateDatetimeString(std::ostringstream &ss)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::tm now_tm;
    localtime_s(&now_tm, &now_c);

    ss << " [" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "]";
}

inline void Logger::generateAlertLevelString(LogLevel level, std::ostringstream &ss) const
{
    if (currentInstance->logLevelDesc.find(level) == logLevelDesc.end()) {
        return;
    }

    ss << " [" << logLevelDesc.at(level) << "]";
}

inline void Logger::generatePid(std::ostringstream &ss)
{
#if defined(_WIN32)
    const DWORD pid = GetCurrentProcessId();
    ss << " [PID: " << std::to_string(pid) << "]";
#endif //_WIN32
}

inline void Logger::LogInst(LogLevel level, const std::vector<enum _logging_type_dest> &type, const std::string &s)
{
    std::ostringstream ss;    
    generateDatetimeString(ss);
    generatePid(ss);
    generateAlertLevelString(level, ss);
    ss << " " << s << std::endl;

    for (std::vector<enum _logging_type_dest>::const_iterator i = type.begin(); i != type.end(); i++) {
        switch (*i) {
        case _logging_type_dest_windows_debug:
            {
                #if defined (_WIN32)       
                OutputDebugStringA(ss.str().c_str());
                #endif //_WIN32
            }
            break;
        case _logging_type_dest_file:
            if (loggingFileName == "") {
                break;
            }
            break;
        case _logging_type_dest_console:
            std::cout << ss.str() << std::endl;
            break;
        case _logging_type_dest_all:
            break;
        default:
            break;
        }
    }
}

template<typename... Args>
inline std::string Logger::formatString(const std::string& format, Args... args) 
{
    const int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    if (size_s <= 0) { 
        return "";
    }
    const size_t size = static_cast<size_t>(size_s);
    const std::unique_ptr<char[]> buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

inline void Logger::Initialize(const std::string &moduleName, std::vector<enum _logging_type_dest> types)
{
    std::lock_guard<std::mutex> lock(logSync);

    if (!currentInstance) {
        currentInstance = std::make_unique<Logger>(moduleName, types);
    }
}

inline void Logger::Initialize(const std::string &moduleName, enum _logging_type_dest loggingType)
{
    Initialize(moduleName, std::vector<enum _logging_type_dest>{ loggingType });
}

inline Logger &Logger::GetInstance(void)
{
    return *currentInstance;
}

template<typename T>
inline Logger &Logger::operator<<(const T &s) {
    std::ostringstream os;
    os << s;
    Log(os.str());
    return *this;
}

inline void Logger::Log(LogLevel level, enum _logging_type_dest type, const std::string &s)
{
    std::lock_guard<std::mutex> lock(logSync);
    if (!currentInstance) {
        return;
    }

    const std::vector<enum _logging_type_dest> types(type);
    currentInstance->LogInst(level, types, s);
}

template<typename... Args>
inline void Logger::Logf(const std::string& format, Args... args) {
    std::string formattedMessage = formatString(format, args...);
    Log(formattedMessage);
}

template<typename... Args>
inline void Logger::Logf(LogLevel level, const std::string& format, Args... args) {
    std::string formattedMessage = formatString(format, args...);
    Log(level, formattedMessage);
}

inline void Logger::Log(LogLevel level, const char *s) 
{
    std::lock_guard<std::mutex> lock(logSync);
    if (!currentInstance) {
        return;
    }

    currentInstance->LogInst(level, currentInstance->loggingTypeDest, s);
}

template<typename T>
inline void Logger::Log(LogLevel level, const T &s)
{
    std::lock_guard<std::mutex> lock(logSync);
    if (!currentInstance) {
        return;
    }

    currentInstance->LogInst(level, currentInstance->loggingTypeDest, s);
}

inline void Logger::Log(const char *s)
{
    Log(currentInstance->defaultLogLevel, s);
}

template<typename T>
inline void Logger::Log(const T &s)
{
    Log(currentInstance->defaultLogLevel, s);
}

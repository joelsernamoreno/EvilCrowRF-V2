// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>

enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class LogCategory
{
    ATTACK,
    SIGNAL,
    SYSTEM,
    HARDWARE,
    NETWORK
};

struct LogEntry
{
    uint32_t timestamp;
    LogLevel level;
    LogCategory category;
    String message;
    DynamicJsonDocument details;

    LogEntry(LogLevel lvl, LogCategory cat, const String &msg)
        : timestamp(millis()), level(lvl), category(cat), message(msg), details(1024) {}
};

class Logger
{
public:
    Logger();

    bool begin(fs::FS &fs);

    void log(LogLevel level, LogCategory category, const String &message);
    void logAttack(const String &attackType, const String &action, const String &details);
    void logSignal(float frequency, int8_t rssi, const String &protocol, const String &details);
    void logError(const String &component, const String &error, const String &details);

    void setLogLevel(LogLevel level) { minimumLevel = level; }
    void enableFileLogging(bool enable) { fileLoggingEnabled = enable; }
    void enableSerialLogging(bool enable) { serialLoggingEnabled = enable; }

    String getLogFilePath(const String &category);

private:
    fs::FS *filesystem;
    LogLevel minimumLevel;
    bool fileLoggingEnabled;
    bool serialLoggingEnabled;

    void writeToFile(const LogEntry &entry);
    void writeToSerial(const LogEntry &entry);
    String getLogLevelString(LogLevel level);
    String getLogCategoryString(LogCategory category);
    void rotateLogFile(const String &path);

    static const size_t MAX_LOG_FILE_SIZE = 1024 * 1024; // 1MB
    static const size_t MAX_LOG_FILES = 5;
};

#endif // LOGGER_H

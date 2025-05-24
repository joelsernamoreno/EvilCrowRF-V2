#include <time.h>

// Time functions if not defined
#ifndef year
int year()
{
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    return timeinfo->tm_year + 1900;
}
#endif

#ifndef month
int month()
{
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    return timeinfo->tm_mon + 1;
}
#endif

#ifndef day
int day()
{
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    return timeinfo->tm_mday;
}
#endif

// Logger.cpp
#include "Logger.h"

Logger::Logger()
    : filesystem(nullptr), minimumLevel(LogLevel::INFO), fileLoggingEnabled(true), serialLoggingEnabled(true) {}

bool Logger::begin(fs::FS &fs)
{
    filesystem = &fs;

    // Create log directories if they don't exist
    if (!filesystem->exists("/logs"))
    {
        filesystem->mkdir("/logs");
    }

    // Create category-specific directories
    const char *categories[] = {"attack", "signal", "system", "hardware", "network"};
    for (const auto &category : categories)
    {
        String path = "/logs/";
        path += category;
        if (!filesystem->exists(path))
        {
            filesystem->mkdir(path);
        }
    }

    return true;
}

void Logger::log(LogLevel level, LogCategory category, const String &message)
{
    if (level < minimumLevel)
        return;

    LogEntry entry(level, category, message);

    if (serialLoggingEnabled)
    {
        writeToSerial(entry);
    }

    if (fileLoggingEnabled)
    {
        writeToFile(entry);
    }
}

void Logger::logAttack(const String &attackType, const String &action, const String &details)
{
    LogEntry entry(LogLevel::INFO, LogCategory::ATTACK, action);
    entry.details["type"] = attackType;
    entry.details["details"] = details;

    if (serialLoggingEnabled)
    {
        writeToSerial(entry);
    }

    if (fileLoggingEnabled)
    {
        writeToFile(entry);
    }
}

void Logger::logSignal(float frequency, int8_t rssi, const String &protocol, const String &details)
{
    LogEntry entry(LogLevel::INFO, LogCategory::SIGNAL, "Signal detected");
    entry.details["frequency"] = frequency;
    entry.details["rssi"] = rssi;
    entry.details["protocol"] = protocol;
    entry.details["details"] = details;

    if (serialLoggingEnabled)
    {
        writeToSerial(entry);
    }

    if (fileLoggingEnabled)
    {
        writeToFile(entry);
    }
}

void Logger::logError(const String &component, const String &error, const String &details)
{
    LogEntry entry(LogLevel::ERROR, LogCategory::SYSTEM, error);
    entry.details["component"] = component;
    entry.details["details"] = details;

    if (serialLoggingEnabled)
    {
        writeToSerial(entry);
    }

    if (fileLoggingEnabled)
    {
        writeToFile(entry);
    }
}

String Logger::getLogFilePath(const String &category)
{
    String timestamp = String(year()) +
                       String(month()) +
                       String(day());

    return "/logs/" + category + "/" + timestamp + ".log";
}

void Logger::writeToFile(const LogEntry &entry)
{
    if (!filesystem)
        return;

    String path = getLogFilePath(getLogCategoryString(entry.category));
    File file = filesystem->open(path, FILE_APPEND);

    if (!file)
    {
        Serial.println("Failed to open log file: " + path);
        return;
    }

    // Check file size and rotate if necessary
    if (file.size() > MAX_LOG_FILE_SIZE)
    {
        file.close();
        rotateLogFile(path);
        file = filesystem->open(path, FILE_APPEND);
    }

    // Create JSON document for log entry
    DynamicJsonDocument doc(2048);
    doc["timestamp"] = entry.timestamp;
    doc["level"] = getLogLevelString(entry.level);
    doc["category"] = getLogCategoryString(entry.category);
    doc["message"] = entry.message;
    doc["details"] = entry.details;

    // Serialize to file
    String output;
    serializeJson(doc, output);
    output += "\n";

    file.print(output);
    file.close();
}

void Logger::writeToSerial(const LogEntry &entry)
{
    String output = "[" + String(entry.timestamp) + "] ";
    output += "[" + getLogLevelString(entry.level) + "] ";
    output += "[" + getLogCategoryString(entry.category) + "] ";
    output += entry.message;

    // Add details if present
    if (!entry.details.isNull())
    {
        output += " - ";
        String details;
        serializeJson(entry.details, details);
        output += details;
    }

    Serial.println(output);
}

String Logger::getLogLevelString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

String Logger::getLogCategoryString(LogCategory category)
{
    switch (category)
    {
    case LogCategory::ATTACK:
        return "attack";
    case LogCategory::SIGNAL:
        return "signal";
    case LogCategory::SYSTEM:
        return "system";
    case LogCategory::HARDWARE:
        return "hardware";
    case LogCategory::NETWORK:
        return "network";
    default:
        return "unknown";
    }
}

void Logger::rotateLogFile(const String &path)
{
    // Rename existing log files
    for (int i = MAX_LOG_FILES - 1; i >= 1; i--)
    {
        String oldName = path + "." + String(i);
        String newName = path + "." + String(i + 1);

        if (filesystem->exists(oldName))
        {
            filesystem->rename(oldName, newName);
        }
    }

    // Rename current log file
    if (filesystem->exists(path))
    {
        filesystem->rename(path, path + ".1");
    }
}

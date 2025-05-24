#ifndef WEATHER_STATION_ATTACK_H
#define WEATHER_STATION_ATTACK_H

#include "AttackManager.h"
#include "RFSignalProcessor.h"
#include "Logger.h"

class WeatherStationAttack
{
public:
    struct Config
    {
        float frequency;
        int8_t modulation;
        uint16_t deviation;
        uint32_t sampleRate;
        bool enableSignalAnalysis;
        uint8_t minSignalStrength;
    };

    struct WeatherData
    {
        float temperature;
        float humidity;
        float pressure;
        float windSpeed;
        float windDirection;
        float rainAmount;
        uint32_t timestamp;
    };

    WeatherStationAttack();
    ~WeatherStationAttack();

    bool init(const Config &config);
    bool startCapture();
    bool stopCapture();
    bool analyze();
    bool spoof(const WeatherData &data);
    bool detectProtocol();

private:
    Config config;
    WeatherData lastData;
    RFSignalProcessor *signalProcessor;
    Logger *logger;

    bool validateWeatherData(const WeatherData &data);
    uint32_t generateChecksum(const WeatherData &data);
    bool encodeWeatherData(const WeatherData &data, uint8_t *buffer, size_t &length);
    bool transmitPacket(const uint8_t *data, size_t length);
    void logAttackStatus(const String &status, const String &details = "");
};

#endif // WEATHER_STATION_ATTACK_H

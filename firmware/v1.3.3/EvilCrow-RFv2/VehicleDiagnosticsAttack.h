#ifndef VEHICLE_DIAGNOSTICS_ATTACK_H
#define VEHICLE_DIAGNOSTICS_ATTACK_H

#include "AttackManager.h"
#include "RFSignalProcessor.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <vector>
#include <map>

class VehicleDiagnosticsAttack
{
public:
    struct TPMSData
    {
        uint32_t sensorId;
        float pressure;
        float temperature;
        uint8_t battery;
        uint32_t timestamp;
    };

    struct VehicleProtocol
    {
        std::string manufacturer;
        uint32_t frequency;
        uint8_t modulation;
        uint16_t bitRate;
        std::vector<uint8_t> preamble;
        uint8_t packetLength;
    };

    struct Config
    {
        uint32_t frequency;
        uint8_t modulation;
        uint16_t bitRate;
        bool enableSignalAnalysis;
        uint8_t minSignalStrength;
        uint16_t captureWindow;
        bool autoDetectProtocol;
    };

    VehicleDiagnosticsAttack();
    ~VehicleDiagnosticsAttack();

    bool init(const Config &config);
    bool startCapture();
    bool stopCapture();
    bool injectTPMSData(const TPMSData &data);
    bool detectVehicleProtocol();

    // Protocol handling
    void addProtocol(const VehicleProtocol &protocol);
    bool selectProtocol(const std::string &manufacturer);

    // Signal analysis
    bool analyzeSignal(std::vector<uint8_t> &signal);
    float validatePressureReading(float pressure);
    bool verifyChecksum(const std::vector<uint8_t> &packet);

    // Attack methods
    bool simulateLowPressure(uint32_t sensorId);
    bool simulateHighPressure(uint32_t sensorId);
    bool simulateSensorMalfunction(uint32_t sensorId);
    bool replayLastPacket();

private:
    Config config;
    RFSignalProcessor *signalProcessor;
    std::vector<VehicleProtocol> supportedProtocols;
    VehicleProtocol *currentProtocol;
    std::map<uint32_t, TPMSData> lastReadings;
    std::vector<uint8_t> lastPacket;

    // Internal methods
    bool encodeTPMSPacket(const TPMSData &data, std::vector<uint8_t> &packet);
    bool transmitPacket(const std::vector<uint8_t> &packet);
    uint32_t calculateChecksum(const std::vector<uint8_t> &data);
    bool validatePacket(const std::vector<uint8_t> &packet);
    void optimizeSignalStrength();
};

#endif // VEHICLE_DIAGNOSTICS_ATTACK_H

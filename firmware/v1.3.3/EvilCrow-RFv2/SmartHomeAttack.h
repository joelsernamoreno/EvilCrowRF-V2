#pragma once
#include "AttackManager.h"
#include "RFSignalProcessor.h"
#include <vector>
#include <memory>

class SmartHomeAttack
{
public:
    struct Config
    {
        uint32_t frequency;
        uint8_t bandwidth;
        uint16_t captureWindow;
        uint8_t minRollingCodes;
        bool activeJamming;
        uint32_t replayDelay;
        bool adaptiveTiming;
        uint8_t signalStrengthThreshold;
    };

    struct CapturedCode
    {
        std::vector<uint8_t> code;
        uint32_t timestamp;
        uint8_t signalStrength;
        bool verified;
        uint16_t pulseLength;
        uint8_t protocol;
    };

    SmartHomeAttack(RFSignalProcessor *processor);
    bool start(const Config &config);
    bool stop();
    bool isRunning() const;
    const std::vector<CapturedCode> &getCapturedCodes() const;
    bool replayCode(size_t index);
    bool replayAllCodes();
    float getSignalStrength() const;

private:
    RFSignalProcessor *rfProcessor;
    std::vector<CapturedCode> capturedCodes;
    Config currentConfig;
    bool running;
    uint32_t lastCaptureTime;
    uint32_t successfulCaptures;
    uint32_t failedCaptures;

    void processCapturedSignal();
    bool isValidRollingCode(const std::vector<uint8_t> &code);
    void startJamming();
    void stopJamming();
    bool verifyCapture(const CapturedCode &code);
    bool detectDeviceType(const CapturedCode &code);
    void optimizeTransmitTiming(CapturedCode &code);
    bool filterDuplicates(const std::vector<uint8_t> &newCode);
};
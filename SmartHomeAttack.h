#SmartHomeAttack.h
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
        // New configuration options
        bool patternLearningEnabled;
        bool multiDeviceCapture;
        uint8_t jammingPowerLevel;
        uint16_t minPatternConfidence;
        uint32_t deviceTrackingDuration;
    };

    struct CapturedCode
    {
        std::vector<uint8_t> code;
        uint32_t timestamp;
        uint8_t signalStrength;
        bool verified;
    };

    struct DevicePattern
    {
        std::vector<uint32_t> timingIntervals;
        std::vector<uint8_t> signalTemplate;
        std::string manufacturerId;
        float confidence;
        uint32_t observationCount;
    };

    struct DeviceCategory
    {
        std::string deviceType;
        uint32_t successfulCaptures;
        uint32_t failedCaptures;
        std::vector<DevicePattern> knownPatterns;
    };

    SmartHomeAttack(RFSignalProcessor *processor);
    bool start(const Config &config);
    bool stop();
    bool isRunning() const;
    const std::vector<CapturedCode> &getCapturedCodes() const;
    bool replayCode(size_t index);
    bool replayAllCodes();

private:
    RFSignalProcessor *rfProcessor;
    std::vector<CapturedCode> capturedCodes;
    Config currentConfig;
    bool running;

    void processCapturedSignal();
    bool isValidRollingCode(const std::vector<uint8_t> &code);
    void startJamming();
    void stopJamming();
    bool verifyCapture(const CapturedCode &code);

    // Advanced pattern learning methods
    bool learnDevicePattern(const CapturedCode &code);
    bool identifyManufacturer(const std::vector<uint8_t> &signal);
    float analyzeTimingPattern(const std::vector<uint32_t> &intervals);

    // Adaptive jamming methods
    void adjustJammingParameters();
    bool startSelectiveJamming(uint32_t targetFreq);
    void updateJammingPower(uint8_t power);

    // Multi-device capture methods
    bool startMultiDeviceCapture();
    void categorizeDevice(const CapturedCode &code);
    float getDeviceSuccessRate(const std::string &deviceType);

    // Code analysis methods
    bool detectEncryption(const std::vector<uint8_t> &signal);
    std::vector<uint8_t> extractKeyComponents(const CapturedCode &code);
    bool matchKnownPatterns(const CapturedCode &code);

    // Advanced replay methods
    bool scheduleReplay(const CapturedCode &code, uint32_t timestamp);
    bool replaySequence(const std::vector<CapturedCode> &sequence);
    void adjustReplayTiming(uint32_t &delay);
};
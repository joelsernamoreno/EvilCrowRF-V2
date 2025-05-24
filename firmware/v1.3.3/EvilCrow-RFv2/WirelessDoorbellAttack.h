#ifndef WIRELESS_DOORBELL_ATTACK_H
#define WIRELESS_DOORBELL_ATTACK_H

#include "AttackManager.h"
#include "RFSignalProcessor.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <vector>
#include <queue>

class WirelessDoorbellAttack
{
public:
    struct TonePattern
    {
        std::vector<uint16_t> frequencies;
        std::vector<uint16_t> durations;
        uint8_t repeatCount;
        std::string description;
    };

    struct DoorbellProfile
    {
        std::string manufacturer;
        uint32_t frequency;
        uint8_t modulation;
        std::vector<uint8_t> identifierCode;
        std::vector<TonePattern> knownPatterns;
    };

    struct Config
    {
        uint32_t frequency;
        uint8_t modulation;
        uint16_t captureWindow;
        bool enablePatternLearning;
        uint8_t minSignalStrength;
        bool autoDetectModel;
    };

    struct ScheduledRing
    {
        TonePattern pattern;
        uint32_t timestamp;
        uint16_t repeatInterval;
        uint8_t remainingRepeats;
    };

    WirelessDoorbellAttack();
    ~WirelessDoorbellAttack();

    bool init(const Config &config);
    bool startCapture();
    bool stopCapture();

    // Pattern analysis and learning
    bool learnDoorbellPattern();
    bool analyzeTonePattern(const std::vector<uint8_t> &signal);
    bool detectDoorbellModel(const std::vector<uint8_t> &signal);

    // Attack methods
    bool triggerDoorbell(const TonePattern &pattern);
    bool injectCustomTone(const std::vector<uint16_t> &frequencies, const std::vector<uint16_t> &durations);
    bool scheduleRing(const TonePattern &pattern, uint32_t timestamp, uint16_t repeatInterval = 0);
    bool jamDoorbellSignal();

    // Pattern management
    void addDoorbellProfile(const DoorbellProfile &profile);
    bool selectDoorbell(const std::string &manufacturer);
    TonePattern createCustomPattern(const std::vector<uint16_t> &freqs, const std::vector<uint16_t> &durations);

    // Status and control
    bool isActive() const;
    void clearScheduledRings();
    bool processScheduledRings();
    size_t getScheduledRingsCount() const;

private:
    Config config;
    RFSignalProcessor *signalProcessor;
    std::vector<DoorbellProfile> knownDoorbells;
    DoorbellProfile *currentDoorbell;
    std::queue<ScheduledRing> scheduledRings;
    bool active;
    TonePattern lastCapturedPattern;

    // Internal methods
    bool validateTonePattern(const TonePattern &pattern);
    bool transmitTonePattern(const TonePattern &pattern);
    void optimizeTransmission();
    bool isValidFrequency(uint16_t freq);
    uint32_t calculateChecksum(const std::vector<uint8_t> &data);
    void updateScheduledRings();
    bool matchDoorbellProfile(const std::vector<uint8_t> &signal);
};

#endif // WIRELESS_DOORBELL_ATTACK_H

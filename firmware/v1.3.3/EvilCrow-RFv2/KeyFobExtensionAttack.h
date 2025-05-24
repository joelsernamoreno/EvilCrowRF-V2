#ifndef KEY_FOB_EXTENSION_ATTACK_H
#define KEY_FOB_EXTENSION_ATTACK_H

#include "AttackManager.h"
#include "RFSignalProcessor.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <vector>
#include <queue>

class KeyFobExtensionAttack
{
public:
    struct Config
    {
        uint32_t frequency;
        uint8_t modulation;
        uint8_t minSignalStrength;
        uint16_t relayDelay;
        bool amplificationEnabled;
        uint8_t amplificationLevel;
        uint16_t maxRelayDistance;
        bool autoFrequencyHopping;
    };

    struct SignalData
    {
        std::vector<uint8_t> rawSignal;
        uint32_t timestamp;
        float signalStrength;
        uint32_t frequency;
    };

    struct RelayStats
    {
        uint32_t packetsRelayed;
        uint32_t failedRelays;
        float averageDelay;
        float maxRange;
        uint32_t lastSuccessTime;
    };

    KeyFobExtensionAttack();
    ~KeyFobExtensionAttack();

    bool init(const Config &config);
    bool startRelay();
    bool stopRelay();
    bool isActive() const;

    // Signal processing methods
    bool processIncomingSignal();
    bool amplifyAndRelay(const SignalData &signal);
    float measureSignalStrength();

    // Range extension methods
    bool extendRange(uint16_t targetDistance);
    void adjustAmplification(uint8_t level);
    bool optimizeRelayDelay();

    // Attack control methods
    bool startRangeExtension();
    bool startSignalAmplification();
    void setFrequencyHopping(bool enabled);

    // Monitoring methods
    RelayStats getRelayStatistics();
    float getEstimatedRange();
    uint32_t getCurrentDelay();

private:
    Config config;
    RFSignalProcessor *signalProcessor;
    std::queue<SignalData> signalQueue;
    RelayStats stats;
    bool active;

    // Internal methods
    bool validateSignal(const SignalData &signal);
    void updateStats(bool success, uint32_t delay);
    bool adjustFrequency(uint32_t targetFreq);
    void optimizeParameters();
    uint8_t calculateOptimalAmplification();
    bool isWithinRange(float signalStrength);
    void cleanupOldSignals();
};

#endif // KEY_FOB_EXTENSION_ATTACK_H

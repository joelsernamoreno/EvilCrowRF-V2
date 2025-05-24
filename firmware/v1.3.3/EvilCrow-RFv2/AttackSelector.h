#ifndef ATTACK_SELECTOR_H
#define ATTACK_SELECTOR_H

#include "SmartHomeAttack.h"
#include "WeatherStationAttack.h"
#include "VehicleDiagnosticsAttack.h"
#include "KeyFobExtensionAttack.h"
#include "RFRemoteOverrideAttack.h"
#include "WirelessDoorbellAttack.h"
#include "AttackManager.h"

enum class AttackType
{
    SMART_HOME,
    WEATHER_STATION,
    VEHICLE_DIAGNOSTICS,
    KEY_FOB_EXTENSION,
    RF_REMOTE_OVERRIDE,
    WIRELESS_DOORBELL
};

class AttackSelector
{
public:
    AttackSelector();
    ~AttackSelector();

    bool initializeAttack(AttackType type);
    bool startAttack(AttackType type);
    bool stopAttack(AttackType type);
    bool isAttackRunning(AttackType type) const;

    // Smart Home Attack methods
    SmartHomeAttack *getSmartHomeAttack() { return smartHomeAttack; }
    bool configureSmartHomeAttack(const SmartHomeAttack::Config &config);

    // Weather Station Attack methods
    WeatherStationAttack *getWeatherStationAttack() { return weatherStationAttack; }
    bool configureWeatherStationAttack(const WeatherStationAttack::Config &config);

    // Vehicle Diagnostics Attack methods
    VehicleDiagnosticsAttack *getVehicleDiagnosticsAttack() { return vehicleDiagnosticsAttack; }
    bool configureVehicleDiagnosticsAttack(const VehicleDiagnosticsAttack::Config &config);

    // Key Fob Extension Attack methods
    KeyFobExtensionAttack *getKeyFobExtensionAttack() { return keyFobExtensionAttack; }
    bool configureKeyFobExtensionAttack(const KeyFobExtensionAttack::Config &config);

    // RF Remote Override Attack methods
    RFRemoteOverrideAttack *getRFRemoteOverrideAttack() { return rfRemoteOverrideAttack; }
    bool configureRFRemoteOverrideAttack(const RFRemoteOverrideAttack::Config &config);

    // Wireless Doorbell Attack methods
    WirelessDoorbellAttack *getWirelessDoorbellAttack() { return wirelessDoorbellAttack; }
    bool configureWirelessDoorbellAttack(const WirelessDoorbellAttack::Config &config);

private:
    SmartHomeAttack *smartHomeAttack;
    WeatherStationAttack *weatherStationAttack;
    VehicleDiagnosticsAttack *vehicleDiagnosticsAttack;
    KeyFobExtensionAttack *keyFobExtensionAttack;
    RFRemoteOverrideAttack *rfRemoteOverrideAttack;
    WirelessDoorbellAttack *wirelessDoorbellAttack;

    AttackType currentAttackType;
    bool attackRunning;

    void cleanupAttacks();
    bool stopCurrentAttack();
    RFSignalProcessor *createSignalProcessor();
};

#endif // ATTACK_SELECTOR_H

#include "AttackSelector.h"
#include "WebRequestHandler.h"

// Attack configuration paths
constexpr char ATTACK_CONFIG_PATH[] = "/attacks/";

// Attack type endpoints
constexpr char SMART_HOME_PATH[] = "/attack/smarthome";
constexpr char WEATHER_STATION_PATH[] = "/attack/weatherstation";
constexpr char VEHICLE_DIAG_PATH[] = "/attack/vehiclediag";
constexpr char KEY_FOB_PATH[] = "/attack/keyfob";
constexpr char RF_REMOTE_PATH[] = "/attack/rfremote";
constexpr char DOORBELL_PATH[] = "/attack/doorbell";

// Global attack selector instance
AttackSelector ATTACK_SELECTOR;

// Add attack routes to web server
void addAttackRoutes(AsyncWebServer &server)
{

    // Smart Home Attack endpoint
    server.on(SMART_HOME_PATH, HTTP_POST, [](AsyncWebServerRequest *request)
              {
        SmartHomeAttack::Config config;
        if (request->hasArg("frequency")) {
            config.frequency = request->arg("frequency").toFloat() * 1000000;
        }
        if (request->hasArg("bandwidth")) {
            config.bandwidth = request->arg("bandwidth").toInt();
        }
        if (request->hasArg("captureWindow")) {
            config.captureWindow = request->arg("captureWindow").toInt();
        }
        if (request->hasArg("minRollingCodes")) {
            config.minRollingCodes = request->arg("minRollingCodes").toInt();
        }
        if (request->hasArg("activeJamming")) {
            config.activeJamming = request->arg("activeJamming") == "true";
        }
        if (request->hasArg("replayDelay")) {
            config.replayDelay = request->arg("replayDelay").toInt();
        }
        if (request->hasArg("adaptiveTiming")) {
            config.adaptiveTiming = request->arg("adaptiveTiming") == "true";
        }
        
        if (ATTACK_SELECTOR.initializeAttack(AttackType::SMART_HOME) &&
            ATTACK_SELECTOR.configureSmartHomeAttack(config)) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });

    // Weather Station Attack endpoint
    server.on(WEATHER_STATION_PATH, HTTP_POST, [](AsyncWebServerRequest *request)
              {
        WeatherStationAttack::Config config;
        if (request->hasArg("frequency")) {
            config.frequency = request->arg("frequency").toFloat() * 1000000;
        }
        if (request->hasArg("modulation")) {
            config.modulation = request->arg("modulation").toInt();
        }
        if (request->hasArg("signalAnalysis")) {
            config.enableSignalAnalysis = request->arg("signalAnalysis") == "true";
        }
        if (request->hasArg("minSignalStrength")) {
            config.minSignalStrength = request->arg("minSignalStrength").toInt();
        }
        
        if (ATTACK_SELECTOR.initializeAttack(AttackType::WEATHER_STATION) &&
            ATTACK_SELECTOR.configureWeatherStationAttack(config)) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });

    // Vehicle Diagnostics Attack endpoint
    server.on(VEHICLE_DIAG_PATH, HTTP_POST, [](AsyncWebServerRequest *request)
              {
        VehicleDiagnosticsAttack::Config config;
        if (request->hasArg("frequency")) {
            config.frequency = request->arg("frequency").toFloat() * 1000000;
        }
        if (request->hasArg("modulation")) {
            config.modulation = request->arg("modulation").toInt();
        }
        if (request->hasArg("bitRate")) {
            config.bitRate = request->arg("bitRate").toInt();
        }
        if (request->hasArg("autoDetectProtocol")) {
            config.autoDetectProtocol = request->arg("autoDetectProtocol") == "true";
        }
        
        if (ATTACK_SELECTOR.initializeAttack(AttackType::VEHICLE_DIAGNOSTICS) &&
            ATTACK_SELECTOR.configureVehicleDiagnosticsAttack(config)) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });

    // Key Fob Extension Attack endpoint
    server.on(KEY_FOB_PATH, HTTP_POST, [](AsyncWebServerRequest *request)
              {
        KeyFobExtensionAttack::Config config;
        if (request->hasArg("frequency")) {
            config.frequency = request->arg("frequency").toFloat() * 1000000;
        }
        if (request->hasArg("modulation")) {
            config.modulation = request->arg("modulation").toInt();
        }
        if (request->hasArg("minSignalStrength")) {
            config.minSignalStrength = request->arg("minSignalStrength").toInt();
        }
        if (request->hasArg("relayDelay")) {
            config.relayDelay = request->arg("relayDelay").toInt();
        }
        if (request->hasArg("amplificationEnabled")) {
            config.amplificationEnabled = request->arg("amplificationEnabled") == "true";
        }
        
        if (ATTACK_SELECTOR.initializeAttack(AttackType::KEY_FOB_EXTENSION) &&
            ATTACK_SELECTOR.configureKeyFobExtensionAttack(config)) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });

    // RF Remote Override Attack endpoint
    server.on(RF_REMOTE_PATH, HTTP_POST, [](AsyncWebServerRequest *request)
              {
        RFRemoteOverrideAttack::Config config;
        if (request->hasArg("frequency")) {
            config.frequency = request->arg("frequency").toFloat() * 1000000;
        }
        if (request->hasArg("modulation")) {
            config.modulation = request->arg("modulation").toInt();
        }
        if (request->hasArg("bitRate")) {
            config.bitRate = request->arg("bitRate").toInt();
        }
        if (request->hasArg("learningMode")) {
            config.enableLearningMode = request->arg("learningMode") == "true";
        }
        
        if (ATTACK_SELECTOR.initializeAttack(AttackType::RF_REMOTE_OVERRIDE) &&
            ATTACK_SELECTOR.configureRFRemoteOverrideAttack(config)) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });

    // Wireless Doorbell Attack endpoint
    server.on(DOORBELL_PATH, HTTP_POST, [](AsyncWebServerRequest *request)
              {
        WirelessDoorbellAttack::Config config;
        if (request->hasArg("frequency")) {
            config.frequency = request->arg("frequency").toFloat() * 1000000;
        }
        if (request->hasArg("modulation")) {
            config.modulation = request->arg("modulation").toInt();
        }
        if (request->hasArg("captureWindow")) {
            config.captureWindow = request->arg("captureWindow").toInt();
        }
        if (request->hasArg("patternLearning")) {
            config.enablePatternLearning = request->arg("patternLearning") == "true";
        }
        
        if (ATTACK_SELECTOR.initializeAttack(AttackType::WIRELESS_DOORBELL) &&
            ATTACK_SELECTOR.configureWirelessDoorbellAttack(config)) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });

    // Generic attack control endpoint
    server.on("/attack/control", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (!request->hasArg("type") || !request->hasArg("action")) {
            request->send(400, "application/json", "{\"error\":\"Missing type or action\"}");
            return;
        }

        String type = request->arg("type");
        String action = request->arg("action");
        AttackType attackType;

        if (type == "smarthome") attackType = AttackType::SMART_HOME;
        else if (type == "weatherstation") attackType = AttackType::WEATHER_STATION;
        else if (type == "vehiclediag") attackType = AttackType::VEHICLE_DIAGNOSTICS;
        else if (type == "keyfob") attackType = AttackType::KEY_FOB_EXTENSION;
        else if (type == "rfremote") attackType = AttackType::RF_REMOTE_OVERRIDE;
        else if (type == "doorbell") attackType = AttackType::WIRELESS_DOORBELL;
        else {
            request->send(400, "application/json", "{\"error\":\"Invalid attack type\"}");
            return;
        }

        bool success = false;
        if (action == "start") {
            success = ATTACK_SELECTOR.startAttack(attackType);
        } else if (action == "stop") {
            success = ATTACK_SELECTOR.stopAttack(attackType);
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid action\"}");
            return;
        }

        if (success) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\"}");
        } });
}

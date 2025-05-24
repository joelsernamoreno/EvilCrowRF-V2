// filepath: /Users/oivindlund/git_repos/EvilCrowRF-V2/firmware/v1.3.3/EvilCrow-RFv2/WebRequestHandler.h
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebSrv.h>

class WebRequestHandler
{
public:
    void init();
    void cleanup();
    bool getParam(AsyncWebServerRequest *request, const char *name, char *buffer, size_t maxLen);
    bool parseRFConfig(const RequestParameters &params, RFConfig &config);
    void handleTxRequest(AsyncWebServerRequest *request);
    void handleRxRequest(AsyncWebServerRequest *request);
    void handleBinaryRequest(AsyncWebServerRequest *request);
    void handleTeslaRequest(AsyncWebServerRequest *request);
    void handleJammerRequest(AsyncWebServerRequest *request);

private:
    RequestParameters *currentRequest;
};

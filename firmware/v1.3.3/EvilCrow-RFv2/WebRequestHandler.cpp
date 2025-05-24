#include "WebRequestHandler.h"
#include "MemoryManager.h"
#include <Arduino.h>

void WebRequestHandler::init()
{
    currentRequest = nullptr;
}

void WebRequestHandler::cleanup()
{
    if (currentRequest)
    {
        if (currentRequest->rawdata)
        {
            MEM_MGR.free(currentRequest->rawdata);
        }
        if (currentRequest->bindata)
        {
            MEM_MGR.free(currentRequest->bindata);
        }
        MEM_MGR.free(currentRequest);
        currentRequest = nullptr;
    }
}

bool WebRequestHandler::getParam(AsyncWebServerRequest *request, const char *name, char *buffer, size_t maxLen)
{
    if (!request->hasParam(name))
        return false;

    String param = request->arg(name);
    if (param.length() >= maxLen)
    {
        log_e("Parameter %s too long: %u >= %u", name, param.length(), maxLen);
        return false;
    }

    strlcpy(buffer, param.c_str(), maxLen);
    return true;
}

bool WebRequestHandler::parseRFConfig(const RequestParameters &params, RFConfig &config)
{
    // Parse frequency
    config.frequency = atof(params.frequency);
    if (config.frequency < 300.0 || config.frequency > 928.0)
    {
        log_e("Invalid frequency: %f", config.frequency);
        return false;
    }

    // Parse modulation
    config.mod = atoi(params.mod);
    if (config.mod < 0 || config.mod > 4)
    {
        log_e("Invalid modulation: %d", config.mod);
        return false;
    }

    // Parse deviation if present
    if (params.deviation[0])
    {
        config.deviation = atof(params.deviation);
    }

    return true;
}

void WebRequestHandler::handleTxRequest(AsyncWebServerRequest *request)
{
    // Clean up any previous request
    cleanup();

    // Allocate new request parameters
    currentRequest = (RequestParameters *)MEM_MGR.allocate(sizeof(RequestParameters));
    if (!currentRequest)
    {
        log_e("Failed to allocate request parameters");
        request->send(500, "text/html", "Memory allocation failed");
        return;
    }

    // Initialize structure
    memset(currentRequest, 0, sizeof(RequestParameters));

    // Get basic parameters
    if (!getParam(request, "frequency", currentRequest->frequency, MAX_FREQUENCY_STR) ||
        !getParam(request, "module", currentRequest->module, MAX_MODULE_STR) ||
        !getParam(request, "mod", currentRequest->mod, MAX_MOD_STR))
    {
        log_e("Missing required parameters");
        request->send(400, "text/html", "Missing required parameters");
        cleanup();
        return;
    }

    // Get optional parameters
    getParam(request, "deviation", currentRequest->deviation, MAX_DEVIATION_STR);
    getParam(request, "transmissions", currentRequest->transmissions, MAX_SAMPLE_STR);

    // Allocate and get raw data
    String rawdata = request->arg("rawdata");
    if (rawdata.length() > 0)
    {
        currentRequest->rawdata = (char *)MEM_MGR.allocate(rawdata.length() + 1);
        if (!currentRequest->rawdata)
        {
            log_e("Failed to allocate raw data buffer");
            request->send(500, "text/html", "Memory allocation failed");
            cleanup();
            return;
        }
        strlcpy(currentRequest->rawdata, rawdata.c_str(), rawdata.length() + 1);
    }

    // Parse RF configuration
    RFConfig config;
    if (!parseRFConfig(*currentRequest, config))
    {
        request->send(400, "text/html", "Invalid RF configuration");
        cleanup();
        return;
    }

    // Process the request...
    // This will be implemented in the main loop to handle the actual RF transmission

    request->send(200, "text/html", "Request accepted");
}

void WebRequestHandler::handleRxRequest(AsyncWebServerRequest *request)
{
    // Similar implementation to handleTxRequest but for RX configuration...
}

void WebRequestHandler::handleBinaryRequest(AsyncWebServerRequest *request)
{
    // Similar implementation to handleTxRequest but for binary data...
}

void WebRequestHandler::handleTeslaRequest(AsyncWebServerRequest *request)
{
    // Similar implementation for Tesla-specific commands...
}

void WebRequestHandler::handleJammerRequest(AsyncWebServerRequest *request)
{
    // Similar implementation for jammer functionality...
}

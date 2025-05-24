#pragma once

#include <ESPAsyncWebServer.h>
#include "MemoryManager.h"

class WebRequestHandler {
public:
    // Request parameter buffer sizes
    static constexpr size_t MAX_FREQUENCY_STR = 16;
    static constexpr size_t MAX_DEVIATION_STR = 16;
    static constexpr size_t MAX_MODULE_STR = 8;
    static constexpr size_t MAX_MOD_STR = 8;
    static constexpr size_t MAX_SAMPLE_STR = 8;
    static constexpr size_t MAX_RAWDATA = 4096;
    static constexpr size_t MAX_BINDATA = 4096;

    struct RequestParameters {
        char frequency[MAX_FREQUENCY_STR];
        char deviation[MAX_DEVIATION_STR];
        char module[MAX_MODULE_STR];
        char mod[MAX_MOD_STR];
        char samplepulse[MAX_SAMPLE_STR];
        char transmissions[MAX_SAMPLE_STR];
        char* rawdata;  // Dynamically allocated
        char* bindata;  // Dynamically allocated
    };

    static WebRequestHandler& getInstance() {
        static WebRequestHandler instance;
        return instance;
    }

    // Initialize request handler
    void init();

    // Handle the various request types
    void handleTxRequest(AsyncWebServerRequest* request);
    void handleRxRequest(AsyncWebServerRequest* request);
    void handleBinaryRequest(AsyncWebServerRequest* request);
    void handleTeslaRequest(AsyncWebServerRequest* request);
    void handleJammerRequest(AsyncWebServerRequest* request);

    // Clean up request resources
    void cleanup();

private:
    WebRequestHandler() {}
    ~WebRequestHandler() { cleanup(); }
    WebRequestHandler(const WebRequestHandler&) = delete;
    WebRequestHandler& operator=(const WebRequestHandler&) = delete;

    // Allocate and copy parameter safely
    bool getParam(AsyncWebServerRequest* request, const char* name, char* buffer, size_t maxLen);
    
    // Parse parameters into RF configuration
    bool parseRFConfig(const RequestParameters& params, RFConfig& config);

    RequestParameters* currentRequest;
};

#define WEB_HANDLER WebRequestHandler::getInstance()

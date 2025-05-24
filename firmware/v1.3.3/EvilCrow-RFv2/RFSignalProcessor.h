#pragma once

#include <Arduino.h>
#include "MemoryManager.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"

class RFSignalProcessor
{
public:
    static constexpr size_t MAX_SAMPLES = 2000;
    static constexpr size_t MIN_SAMPLES = 30;
    static constexpr uint32_t MIN_PULSE_WIDTH = 100; // Minimum valid pulse width in microseconds

    struct SignalBuffer
    {
        unsigned long *raw;      // Raw timing samples
        unsigned long *smooth;   // Smoothed timing samples
        size_t rawCount;         // Number of raw samples
        size_t smoothCount;      // Number of smoothed samples
        uint32_t errorTolerance; // Error tolerance for smoothing
    };

    static RFSignalProcessor &getInstance()
    {
        static RFSignalProcessor instance;
        return instance;
    }

    bool init();
    void cleanup();

    // Signal capture
    bool startCapture();
    bool stopCapture();
    void processPulse(unsigned long duration);

    // Signal processing
    bool smoothSignal();
    bool compressSignal();
    bool decodeBinary();

    // Signal transmission
    bool transmitRaw(const long *timings, size_t count, int repetitions);
    bool transmitBinary(const uint8_t *data, size_t len, uint32_t bitTime);

    // Signal analysis
    bool analyzePulses(uint32_t &period, uint32_t &zeroPulse, uint32_t &onePulse);
    float calculateSignalQuality();

    // Buffer management
    SignalBuffer *getBuffer() { return &signalBuffer; }

private:
    RFSignalProcessor() {}
    ~RFSignalProcessor() { cleanup(); }
    RFSignalProcessor(const RFSignalProcessor &) = delete;
    RFSignalProcessor &operator=(const RFSignalProcessor &) = delete;

    SignalBuffer signalBuffer;
    bool isCapturing;
    unsigned long lastPulseTime;
};

#define RF_PROCESSOR RFSignalProcessor::getInstance()

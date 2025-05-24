#pragma once

#include <Arduino.h>
#include "MemoryManager.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"

class RFSignalProcessor
{
public:
    static constexpr size_t MAX_SAMPLES = 2000;
    static constexpr size_t MIN_SAMPLES = 30;
    static constexpr uint32_t MIN_PULSE_WIDTH = 100;
    static constexpr size_t MAX_PATTERN_LENGTH = 64;
    static constexpr float MIN_PATTERN_CONFIDENCE = 0.85f;

    // Signal modulation types
    enum class ModulationType
    {
        UNKNOWN,
        OOK, // On-Off Keying
        ASK, // Amplitude Shift Keying
        FSK  // Frequency Shift Keying
    };

    struct SignalBuffer
    {
        unsigned long *raw;
        unsigned long *smooth;
        size_t rawCount;
        size_t smoothCount;
        uint32_t errorTolerance;
    };

    struct ProtocolInfo
    {
        const char *name;
        uint32_t baseUnit;
        float tolerance;
        bool isInverted;
        uint8_t minRepeats;
    };

    struct PatternInfo
    {
        uint32_t length;
        uint32_t repeats;
        float confidence;
        bool isValid;
    };

    struct SpectralInfo
    {
        float dominantFreq;            // Dominant frequency in Hz
        float signalToNoise;           // Signal-to-noise ratio in dB
        float bandwidth;               // Signal bandwidth in Hz
        uint32_t symbolRate;           // Estimated symbol rate
        float peakToPeakJitter;        // Jitter as percentage
        float dutyCycle;               // Duty cycle as percentage
        float phaseConsistency;        // Phase consistency score
        float frequencyStability;      // Frequency stability score
        float signalPurity;            // Signal purity metric
        float modulationIndex;         // Modulation index for FSK
        ModulationType modulationType; // Detected modulation type
    };

    struct QualityMetrics
    {
        float overall;     // Overall signal quality (0-100)
        float noiseLevel;  // Noise level as percentage
        float glitchLevel; // Glitch occurrence rate
        float jitterLevel; // Timing jitter level
        float stability;   // Signal stability score
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

    // Enhanced signal analysis
    bool analyzePulses(uint32_t &period, uint32_t &zeroPulse, uint32_t &onePulse);
    float calculateSignalQuality();
    bool detectPattern(PatternInfo &pattern);
    bool identifyProtocol(ProtocolInfo &protocol);
    bool analyzeSpectrum(SpectralInfo &spectrum);
    bool analyzeSignalQuality(QualityMetrics &metrics);
    bool validateTiming(uint32_t timing, uint32_t expected, float tolerance);

    // Buffer management
    SignalBuffer *getBuffer() { return &signalBuffer; }

private:
    RFSignalProcessor() {}
    ~RFSignalProcessor() { cleanup(); }
    RFSignalProcessor(const RFSignalProcessor &) = delete;
    RFSignalProcessor &operator=(const RFSignalProcessor &) = delete;

    // Private helper methods
    bool findRepeatingPattern(const unsigned long *data, size_t count, PatternInfo &pattern);
    float calculateProtocolConfidence(const ProtocolInfo &protocol);
    float calculateSpectralMatch(const ProtocolInfo &protocol, const SpectralInfo &spectrum);
    void calculateSpectralProperties(SpectralInfo &spectrum);
    ModulationType identifyModulationType(float avgWidth, float variance, float dynamicRange);

    SignalBuffer signalBuffer;
    bool isCapturing;
    unsigned long lastPulseTime;
};

#define RF_PROCESSOR RFSignalProcessor::getInstance()

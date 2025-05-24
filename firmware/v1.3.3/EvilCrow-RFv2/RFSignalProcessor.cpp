#include "RFSignalProcessor.h"
#include "MemoryManager.h"
#include <complex>

bool RFSignalProcessor::init()
{
    // Allocate memory for signal buffers
    signalBuffer.raw = (unsigned long *)MEM_MGR.getSamplePool()->allocate(MAX_SAMPLES * sizeof(unsigned long));
    signalBuffer.smooth = (unsigned long *)MEM_MGR.getSamplePool()->allocate(MAX_SAMPLES * sizeof(unsigned long));

    if (!signalBuffer.raw || !signalBuffer.smooth)
    {
        log_e("Failed to allocate signal buffers");
        cleanup();
        return false;
    }

    signalBuffer.rawCount = 0;
    signalBuffer.smoothCount = 0;
    signalBuffer.errorTolerance = 200; // Default error tolerance

    isCapturing = false;
    lastPulseTime = 0;

    return true;
}

void RFSignalProcessor::cleanup()
{
    if (signalBuffer.raw)
    {
        MEM_MGR.getSamplePool()->free(signalBuffer.raw);
        signalBuffer.raw = nullptr;
    }

    if (signalBuffer.smooth)
    {
        MEM_MGR.getSamplePool()->free(signalBuffer.smooth);
        signalBuffer.smooth = nullptr;
    }

    signalBuffer.rawCount = 0;
    signalBuffer.smoothCount = 0;
}

bool RFSignalProcessor::startCapture()
{
    if (!signalBuffer.raw)
    {
        log_e("Signal buffer not initialized");
        return false;
    }

    signalBuffer.rawCount = 0;
    isCapturing = true;
    lastPulseTime = micros();
    return true;
}

bool RFSignalProcessor::stopCapture()
{
    isCapturing = false;
    return signalBuffer.rawCount >= MIN_SAMPLES;
}

void RFSignalProcessor::processPulse(unsigned long duration)
{
    if (!isCapturing || !signalBuffer.raw || signalBuffer.rawCount >= MAX_SAMPLES)
    {
        return;
    }

    // Filter out too short pulses
    if (duration < MIN_PULSE_WIDTH)
    {
        return;
    }

    signalBuffer.raw[signalBuffer.rawCount++] = duration;
}

bool RFSignalProcessor::smoothSignal()
{
    if (!signalBuffer.raw || !signalBuffer.smooth || signalBuffer.rawCount < MIN_SAMPLES)
    {
        log_e("Invalid buffer state for smoothing");
        return false;
    }

    // Enhanced signal smoothing with adaptive window
    const size_t maxWindow = 5;
    size_t windowSize = min(maxWindow, signalBuffer.rawCount / 10);

    // Calculate dynamic error tolerance based on signal properties
    uint32_t minPulse = UINT32_MAX;
    uint32_t maxPulse = 0;

    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        minPulse = min(minPulse, signalBuffer.raw[i]);
        maxPulse = max(maxPulse, signalBuffer.raw[i]);
    }

    // Adjust error tolerance based on signal range
    float dynamicTolerance = (maxPulse - minPulse) * 0.1f; // 10% of range
    signalBuffer.errorTolerance = constrain(
        (uint32_t)dynamicTolerance,
        100u, // Minimum tolerance
        500u  // Maximum tolerance
    );

    // Moving average smoothing with edge preservation
    signalBuffer.smoothCount = 0;
    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        // Calculate window bounds
        size_t windowStart = (i >= windowSize) ? i - windowSize : 0;
        size_t windowEnd = min(i + windowSize + 1, signalBuffer.rawCount);

        // Calculate weighted average
        uint32_t sum = 0;
        uint32_t weight = 0;
        uint32_t totalWeight = 0;

        for (size_t j = windowStart; j < windowEnd; j++)
        {
            // Use Gaussian-like weighting
            weight = windowSize - abs((long)(i - j));
            sum += signalBuffer.raw[j] * weight;
            totalWeight += weight;
        }

        uint32_t smoothed = sum / totalWeight;

        // Edge preservation: don't smooth if it would create a new edge
        bool isEdge = (i > 0) &&
                      abs((long)(signalBuffer.raw[i] - signalBuffer.raw[i - 1])) >
                          signalBuffer.errorTolerance;

        if (isEdge)
        {
            smoothed = signalBuffer.raw[i];
        }

        // Only keep pulses that meet our criteria
        if (smoothed >= MIN_PULSE_WIDTH)
        {
            signalBuffer.smooth[signalBuffer.smoothCount++] = smoothed;
        }
    }

    return signalBuffer.smoothCount >= MIN_SAMPLES;
}

bool RFSignalProcessor::compressSignal()
{
    if (!signalBuffer.smooth || signalBuffer.smoothCount < MIN_SAMPLES)
    {
        log_e("Invalid buffer state for compression");
        return false;
    }

    // Enhanced compression with pattern preservation
    const float toleranceRatio = 0.15f; // 15% tolerance
    size_t newCount = 0;
    unsigned long lastPulse = signalBuffer.smooth[0];
    int similarCount = 1;
    unsigned long sum = lastPulse;

    // Track pattern metrics
    uint32_t minPeriod = UINT32_MAX;
    uint32_t maxPeriod = 0;

    for (size_t i = 1; i < signalBuffer.smoothCount; i++)
    {
        unsigned long currentPulse = signalBuffer.smooth[i];
        uint32_t tolerance = max(
            (uint32_t)(lastPulse * toleranceRatio),
            signalBuffer.errorTolerance);

        // Calculate period for pattern preservation
        if (i > 1)
        {
            uint32_t period = signalBuffer.smooth[i] + signalBuffer.smooth[i - 1];
            minPeriod = min(minPeriod, period);
            maxPeriod = max(maxPeriod, period);
        }

        if (abs((long)(currentPulse - lastPulse)) < tolerance)
        {
            // Merge similar pulses
            sum += currentPulse;
            similarCount++;
        }
        else
        {
            // Store merged pulse
            unsigned long averagePulse = sum / similarCount;

            // Preserve timing relationships
            if (minPeriod != UINT32_MAX && maxPeriod > 0)
            {
                uint32_t expectedPeriod = (minPeriod + maxPeriod) / 2;
                if (abs((long)(averagePulse - expectedPeriod)) < tolerance)
                {
                    averagePulse = expectedPeriod;
                }
            }

            signalBuffer.smooth[newCount++] = averagePulse;
            lastPulse = currentPulse;
            sum = currentPulse;
            similarCount = 1;
        }
    }

    // Store the last group
    if (similarCount > 0)
    {
        signalBuffer.smooth[newCount++] = sum / similarCount;
    }

    signalBuffer.smoothCount = newCount;
    return true;
}

bool RFSignalProcessor::transmitRaw(const long *timings, size_t count, int repetitions)
{
    if (!timings || count == 0)
    {
        return false;
    }

    // Configure CC1101 for transmission
    ELECHOUSE_cc1101.SetTx();

    // Calculate optimal transmission parameters
    uint32_t totalDuration = 0;
    uint32_t minTiming = UINT32_MAX;
    for (size_t i = 0; i < count; i++)
    {
        totalDuration += timings[i];
        minTiming = min((uint32_t)timings[i], minTiming);
    }

    // Adjust timing precision based on minimum pulse width
    uint32_t precisionDelay = minTiming < 100 ? 1 : 5;

    // Add transmission guard time
    const uint32_t guardTime = 2000; // 2ms guard time

    // Transmit the signal
    for (int rep = 0; rep < repetitions; rep++)
    {
        // Start transmission with guard time
        delayMicroseconds(guardTime);

        for (size_t i = 0; i < count; i++)
        {
            if (i % 2 == 0)
            {
                digitalWrite(CC1101_TX_PIN, HIGH);
            }
            else
            {
                digitalWrite(CC1101_TX_PIN, LOW);
            }

            // Precise timing control
            uint32_t duration = timings[i];
            if (duration > 1000)
            {
                // For longer pulses, use a combination of delay() and delayMicroseconds()
                delay(duration / 1000);
                duration %= 1000;
            }

            // Fine-grained delay with busy wait for short durations
            if (duration < 100)
            {
                uint32_t start = micros();
                while (micros() - start < duration)
                {
                    // Busy wait for maximum precision
                }
            }
            else
            {
                delayMicroseconds(duration);
            }
        }

        // End transmission with guard time
        digitalWrite(CC1101_TX_PIN, LOW);
        delayMicroseconds(guardTime);
    }

    // Return to receive mode
    ELECHOUSE_cc1101.SetRx();
    return true;
}

bool RFSignalProcessor::transmitBinary(const uint8_t *data, size_t len, uint32_t bitTime)
{
    if (!data || len == 0)
    {
        return false;
    }

    // Configure CC1101 for transmission
    ELECHOUSE_cc1101.SetTx();

    // Calculate timing parameters
    const uint32_t shortPulse = bitTime;
    const uint32_t longPulse = bitTime * 3; // Typical for many protocols
    const uint32_t guardTime = bitTime * 4; // Guard time between repetitions

    // Transmit preamble
    digitalWrite(CC1101_TX_PIN, HIGH);
    delayMicroseconds(longPulse);
    digitalWrite(CC1101_TX_PIN, LOW);
    delayMicroseconds(shortPulse);

    // Transmit data
    for (size_t byte = 0; byte < len; byte++)
    {
        uint8_t current = data[byte];

        // Transmit each bit
        for (int bit = 7; bit >= 0; bit--)
        {
            bool isOne = (current & (1 << bit)) != 0;

            // Manchester encoding
            if (isOne)
            {
                digitalWrite(CC1101_TX_PIN, HIGH);
                delayMicroseconds(shortPulse);
                digitalWrite(CC1101_TX_PIN, LOW);
                delayMicroseconds(shortPulse);
            }
            else
            {
                digitalWrite(CC1101_TX_PIN, LOW);
                delayMicroseconds(shortPulse);
                digitalWrite(CC1101_TX_PIN, HIGH);
                delayMicroseconds(shortPulse);
            }
        }
    }

    // Transmit end marker
    digitalWrite(CC1101_TX_PIN, LOW);
    delayMicroseconds(guardTime);

    // Return to receive mode
    ELECHOUSE_cc1101.SetRx();
    return true;
}

bool RFSignalProcessor::decodeBinary()
{
    if (!signalBuffer.smooth || signalBuffer.smoothCount < MIN_SAMPLES)
    {
        return false;
    }

    // Find pulse width characteristics
    uint32_t period, zeroPulse, onePulse;
    if (!analyzePulses(period, zeroPulse, onePulse))
    {
        return false;
    }

    // Detect modulation type for appropriate decoding
    SpectralInfo spectrum;
    analyzeSpectrum(spectrum);

    // Adjust decoding based on modulation
    float threshold = 0.0f;
    switch (spectrum.modulationType)
    {
    case ModulationType::OOK:
        threshold = (zeroPulse + onePulse) / 2.0f;
        break;

    case ModulationType::ASK:
        threshold = onePulse * 0.7f; // 70% of high level
        break;

    case ModulationType::FSK:
        // For FSK, compare against the detected frequency bands
        threshold = period * 0.75f;
        break;

    default:
        // Default to OOK/ASK style decoding
        threshold = (zeroPulse + onePulse) / 2.0f;
        break;
    }

    // Initialize bit buffer (to be implemented based on needs)
    // uint8_t bitBuffer[MAX_SAMPLES/8] = {0};
    // size_t bitCount = 0;

    // Pattern detection for protocol identification
    PatternInfo pattern;
    bool hasPattern = detectPattern(pattern);

    if (hasPattern)
    {
        // Use pattern length for frame synchronization
        // Implementation specific to protocol requirements
        return true;
    }

    return false;
}

bool RFSignalProcessor::analyzePulses(uint32_t &period, uint32_t &zeroPulse, uint32_t &onePulse)
{
    if (!signalBuffer.smooth || signalBuffer.smoothCount < MIN_SAMPLES)
    {
        return false;
    }

    // Find the most common pulse widths
    // This is a simplified implementation - real one would use more sophisticated analysis
    period = 0;
    for (size_t i = 0; i < signalBuffer.smoothCount - 1; i += 2)
    {
        period += signalBuffer.smooth[i] + signalBuffer.smooth[i + 1];
    }
    period /= (signalBuffer.smoothCount / 2);

    // Analyze pulse widths to identify 0s and 1s
    // This is a placeholder - real implementation would be more complex
    zeroPulse = period / 3;
    onePulse = period * 2 / 3;

    return true;
}

float RFSignalProcessor::calculateSignalQuality()
{
    if (!signalBuffer.raw || signalBuffer.rawCount < MIN_SAMPLES)
    {
        return 0.0f;
    }

    // Calculate standard deviation as a quality metric
    unsigned long sum = 0, sumSq = 0;
    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        sum += signalBuffer.raw[i];
        sumSq += signalBuffer.raw[i] * signalBuffer.raw[i];
    }

    float avg = (float)sum / signalBuffer.rawCount;
    float variance = ((float)sumSq / signalBuffer.rawCount) - (avg * avg);
    float stdDev = sqrt(variance);

    // Convert to a quality percentage (lower deviation = higher quality)
    float quality = 100.0f * (1.0f - (stdDev / avg));
    return constrain(quality, 0.0f, 100.0f);
}

bool RFSignalProcessor::detectPattern(PatternInfo &pattern)
{
    if (!signalBuffer.smooth || signalBuffer.smoothCount < MIN_SAMPLES)
    {
        return false;
    }

    pattern.isValid = false;

    // Try to find repeating patterns in the smooth buffer
    if (!findRepeatingPattern(signalBuffer.smooth, signalBuffer.smoothCount, pattern))
    {
        return false;
    }

    // Validate the detected pattern
    if (pattern.confidence >= MIN_PATTERN_CONFIDENCE && pattern.repeats >= 2)
    {
        pattern.isValid = true;
        return true;
    }

    return false;
}

bool RFSignalProcessor::findRepeatingPattern(const unsigned long *data, size_t count, PatternInfo &pattern)
{
    pattern.length = 0;
    pattern.repeats = 0;
    pattern.confidence = 0.0f;

    // Try different pattern lengths
    for (size_t len = 4; len <= MAX_PATTERN_LENGTH && len <= count / 2; len *= 2)
    {
        size_t matches = 0;
        size_t total = count / len;

        for (size_t i = 0; i < total - 1; i++)
        {
            bool matched = true;
            for (size_t j = 0; j < len; j++)
            {
                if (!validateTiming(data[i * len + j], data[(i + 1) * len + j], 0.1f))
                {
                    matched = false;
                    break;
                }
            }
            if (matched)
                matches++;
        }

        float confidence = (float)matches / (total - 1);
        if (confidence > pattern.confidence)
        {
            pattern.length = len;
            pattern.repeats = matches + 1;
            pattern.confidence = confidence;
        }
    }

    return pattern.length > 0;
}

bool RFSignalProcessor::identifyProtocol(ProtocolInfo &protocol)
{
    static const ProtocolInfo knownProtocols[] = {
        // Standard protocols
        {"NEC", 562, 0.15f, false, 2},     // NEC IR protocol
        {"RC5", 889, 0.15f, true, 2},      // RC5 protocol
        {"PT2262", 350, 0.20f, false, 4},  // PT2262/EV1527
        {"CAME", 320, 0.20f, false, 4},    // CAME remote controls
        {"Holtek", 366, 0.15f, false, 4},  // HT12E/HT12D protocol
        {"SMC5326", 368, 0.15f, false, 4}, // SMC5326 format
        {"Tesla", 410, 0.15f, false, 3},   // Tesla charge port
        {"Keeloq", 400, 0.20f, false, 4},  // Keeloq protocol
        {"Nice", 500, 0.15f, false, 4},    // Nice remote controls
        {"BFT", 512, 0.15f, false, 4}      // BFT automation
    };

    float bestConfidence = 0.0f;
    const ProtocolInfo *bestMatch = nullptr;
    SpectralInfo spectrum;

    // Get spectral properties for advanced protocol matching
    analyzeSpectrum(spectrum);

    // Enhanced protocol detection with multiple metrics
    for (const auto &p : knownProtocols)
    {
        // Calculate basic timing confidence
        float timingConfidence = calculateProtocolConfidence(p);

        // Calculate pattern match confidence
        PatternInfo pattern;
        float patternConfidence = 0.0f;
        if (detectPattern(pattern))
        {
            patternConfidence = pattern.confidence;
        }

        // Calculate spectral confidence
        float spectralConfidence = calculateSpectralMatch(p, spectrum);

        // Combine confidences with weighted average
        float totalConfidence = (timingConfidence * 0.4f) +
                                (patternConfidence * 0.3f) +
                                (spectralConfidence * 0.3f);

        if (totalConfidence > bestConfidence && totalConfidence >= 0.75f)
        {
            bestConfidence = totalConfidence;
            bestMatch = &p;
        }
    }

    if (bestMatch)
    {
        protocol = *bestMatch;
        return true;
    }

    return false;
}

float RFSignalProcessor::calculateProtocolConfidence(const ProtocolInfo &protocol)
{
    if (!signalBuffer.smooth || signalBuffer.smoothCount < MIN_SAMPLES)
    {
        return 0.0f;
    }

    size_t matches = 0;
    size_t expectedPatternCount = 0;
    size_t repeatCount = 0;
    bool lastWasMatch = false;
    float timingAccuracy = 0.0f;

    // Enhanced timing analysis with statistical validation
    for (size_t i = 0; i < signalBuffer.smoothCount; i++)
    {
        uint32_t timing = signalBuffer.smooth[i];
        bool matched = false;
        float bestAccuracy = 0.0f;

        // Check common multiples of the base unit
        for (uint32_t mult = 1; mult <= 4; mult++)
        {
            uint32_t expectedTiming = protocol.baseUnit * mult;
            if (validateTiming(timing, expectedTiming, protocol.tolerance))
            {
                matched = true;
                float accuracy = 1.0f - (abs((long)(timing - expectedTiming)) / (float)expectedTiming);
                bestAccuracy = max(bestAccuracy, accuracy);
                if (lastWasMatch)
                {
                    repeatCount++;
                }
                break;
            }
        }

        if (matched)
        {
            matches++;
            timingAccuracy += bestAccuracy;
            if (!lastWasMatch)
            {
                expectedPatternCount++;
            }
        }
        lastWasMatch = matched;
    }

    // Calculate weighted confidence score
    float matchRatio = (float)matches / signalBuffer.smoothCount;
    float accuracyScore = matches > 0 ? timingAccuracy / matches : 0.0f;
    float repeatScore = repeatCount >= protocol.minRepeats ? 1.0f : (float)repeatCount / protocol.minRepeats;

    // Combine metrics with weights
    float confidence = (matchRatio * 0.5f) + (accuracyScore * 0.3f) + (repeatScore * 0.2f);

    return confidence;
}

float RFSignalProcessor::calculateSpectralMatch(const ProtocolInfo &protocol, const SpectralInfo &spectrum)
{
    // Calculate expected spectral properties based on protocol
    float expectedSymbolRate = 1000000.0f / (protocol.baseUnit * 2); // Rough estimate
    float expectedBandwidth = expectedSymbolRate * 1.2f;             // Typical bandwidth

    // Compare actual vs expected properties
    float symbolRateMatch = 1.0f - min(abs(spectrum.symbolRate - expectedSymbolRate) / expectedSymbolRate, 1.0f);
    float bandwidthMatch = 1.0f - min(abs(spectrum.bandwidth - expectedBandwidth) / expectedBandwidth, 1.0f);

    // Additional metrics
    float snrScore = min(spectrum.signalToNoise / 30.0f, 1.0f); // Normalize SNR (assuming 30dB is excellent)
    float phaseScore = spectrum.phaseConsistency / 100.0f;
    float modulationScore = 1.0f - min(abs(spectrum.modulationIndex - 1.0f), 1.0f);

    // Combine scores with weights
    float confidence = (symbolRateMatch * 0.3f) +
                       (bandwidthMatch * 0.2f) +
                       (snrScore * 0.2f) +
                       (phaseScore * 0.15f) +
                       (modulationScore * 0.15f);

    return confidence;
}

bool RFSignalProcessor::analyzeSpectrum(SpectralInfo &spectrum)
{
    if (!signalBuffer.raw || signalBuffer.rawCount < MIN_SAMPLES)
    {
        return false;
    }

    calculateSpectralProperties(spectrum);
    return true;
}

void RFSignalProcessor::calculateSpectralProperties(SpectralInfo &spectrum)
{
    if (!signalBuffer.raw || signalBuffer.rawCount < MIN_SAMPLES)
    {
        spectrum = {};
        return;
    }

    // Enhanced timing analysis
    uint32_t totalWidth = 0;
    float totalVariance = 0;
    uint32_t minWidth = UINT32_MAX;
    uint32_t maxWidth = 0;

    // First pass: calculate mean and find min/max
    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        totalWidth += signalBuffer.raw[i];
        minWidth = min(minWidth, signalBuffer.raw[i]);
        maxWidth = max(maxWidth, signalBuffer.raw[i]);
    }
    float avgWidth = (float)totalWidth / signalBuffer.rawCount;

    // Second pass: calculate enhanced metrics
    float totalSquaredError = 0;
    float totalPhaseError = 0;
    uint32_t totalHigh = 0;
    uint32_t consecutiveJitter = 0;
    uint32_t maxConsecutiveJitter = 0;

    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        // Variance calculation
        float diff = signalBuffer.raw[i] - avgWidth;
        totalSquaredError += diff * diff;

        // Phase consistency calculation
        if (i >= 2 && (i % 2) == 0)
        {
            float expectedPhase = signalBuffer.raw[i - 2];
            float actualPhase = signalBuffer.raw[i];
            totalPhaseError += abs(actualPhase - expectedPhase) / expectedPhase;
        }

        // Duty cycle calculation (for even indices - high pulses)
        if ((i % 2) == 0)
        {
            totalHigh += signalBuffer.raw[i];
        }

        // Jitter analysis
        if (i > 0)
        {
            float jitter = abs((long)(signalBuffer.raw[i] - signalBuffer.raw[i - 1])) / avgWidth;
            if (jitter > 0.1f)
            { // 10% threshold
                consecutiveJitter++;
                maxConsecutiveJitter = max(maxConsecutiveJitter, consecutiveJitter);
            }
            else
            {
                consecutiveJitter = 0;
            }
        }
    }

    float variance = totalSquaredError / signalBuffer.rawCount;

    // Calculate advanced metrics
    spectrum.dominantFreq = 1000000.0f / (2 * avgWidth); // Base frequency
    spectrum.bandwidth = 1000000.0f / (2 * sqrt(variance));

    // Enhanced SNR calculation using RMS
    float rms = sqrt(totalSquaredError / signalBuffer.rawCount);
    float noiseFloor = minWidth * 0.1f; // Estimate noise floor as 10% of min width
    spectrum.signalToNoise = 20 * log10((maxWidth - minWidth) / (2 * rms + noiseFloor));

    // Calculate symbol rate from detected patterns
    PatternInfo pattern;
    if (detectPattern(pattern) && pattern.length > 0)
    {
        float bitPeriod = 2 * avgWidth; // Assuming Manchester encoding
        spectrum.symbolRate = (uint32_t)(1000000.0f / bitPeriod);
    }
    else
    {
        spectrum.symbolRate = 0;
    }

    // Additional advanced metrics
    spectrum.peakToPeakJitter = 100 * (maxWidth - minWidth) / avgWidth;
    spectrum.dutyCycle = 100 * ((float)totalHigh / totalWidth);
    spectrum.phaseConsistency = 100 * (1.0f - (totalPhaseError / (signalBuffer.rawCount / 2)));
    spectrum.frequencyStability = 100 * (1.0f - sqrt(variance) / avgWidth);

    // Signal quality metrics
    float dynamicRange = (float)maxWidth / minWidth;
    spectrum.signalPurity = 100 * (1.0f / (1.0f + abs(dynamicRange - 2.0f))); // Ideal ratio is 2.0

    // Modulation analysis
    if (spectrum.bandwidth > 0)
    {
        // Estimate modulation index based on timing variations
        spectrum.modulationIndex = spectrum.dominantFreq / spectrum.bandwidth;

        // Detect modulation type based on timing patterns
        spectrum.modulationType = identifyModulationType(avgWidth, variance, dynamicRange);
    }
}

RFSignalProcessor::ModulationType RFSignalProcessor::identifyModulationType(float avgWidth, float variance, float dynamicRange)
{
    // Modulation type detection based on signal characteristics
    float varianceRatio = sqrt(variance) / avgWidth;
    float idealFskRatio = 2.0f; // Ideal ratio for FSK
    float idealAskRatio = 3.0f; // Ideal ratio for ASK

    if (varianceRatio < 0.1f)
    {
        return ModulationType::OOK; // On-Off Keying (clean)
    }
    else if (abs(dynamicRange - idealFskRatio) < 0.5f)
    {
        return ModulationType::FSK; // Frequency Shift Keying
    }
    else if (abs(dynamicRange - idealAskRatio) < 0.5f)
    {
        return ModulationType::ASK; // Amplitude Shift Keying
    }
    else
    {
        return ModulationType::UNKNOWN;
    }
}

bool RFSignalProcessor::analyzeSignalQuality(QualityMetrics &metrics)
{
    if (!signalBuffer.raw || signalBuffer.rawCount < MIN_SAMPLES)
    {
        return false;
    }

    // Calculate base signal quality
    metrics.overall = calculateSignalQuality();

    // Enhanced noise analysis
    uint32_t noiseCount = 0;
    uint32_t glitchCount = 0;
    float totalJitter = 0;

    float avgPulseWidth = 0;
    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        avgPulseWidth += signalBuffer.raw[i];
    }
    avgPulseWidth /= signalBuffer.rawCount;

    // Analyze each pulse for anomalies
    for (size_t i = 1; i < signalBuffer.rawCount; i++)
    {
        // Detect noise (very short pulses)
        if (signalBuffer.raw[i] < MIN_PULSE_WIDTH)
        {
            noiseCount++;
        }

        // Detect glitches (sudden large variations)
        float ratio = (float)signalBuffer.raw[i] / signalBuffer.raw[i - 1];
        if (ratio < 0.25f || ratio > 4.0f)
        {
            glitchCount++;
        }

        // Calculate timing jitter
        if ((i % 2) == 0)
        { // Compare similar polarity pulses
            float jitter = abs(signalBuffer.raw[i] - signalBuffer.raw[i - 2]) / (float)signalBuffer.raw[i - 2];
            totalJitter += jitter;
        }
    }

    // Calculate quality metrics
    metrics.noiseLevel = 100.0f * noiseCount / signalBuffer.rawCount;
    metrics.glitchLevel = 100.0f * glitchCount / signalBuffer.rawCount;
    metrics.jitterLevel = 100.0f * totalJitter / (signalBuffer.rawCount / 2);

    // Calculate signal stability
    float stabilityScore = 100.0f - (metrics.noiseLevel * 0.4f +
                                     metrics.glitchLevel * 0.4f +
                                     metrics.jitterLevel * 0.2f);
    metrics.stability = max(0.0f, min(100.0f, stabilityScore));

    return true;
}

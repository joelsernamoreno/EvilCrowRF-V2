#include "RFSignalProcessor.h"

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

    // Calculate average
    unsigned long average = 0;
    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        average += signalBuffer.raw[i];
    }
    average /= signalBuffer.rawCount;

    // Smooth the signal
    signalBuffer.smoothCount = 0;
    for (size_t i = 0; i < signalBuffer.rawCount; i++)
    {
        if (abs((long)(signalBuffer.raw[i] - average)) < signalBuffer.errorTolerance)
        {
            signalBuffer.smooth[signalBuffer.smoothCount++] = signalBuffer.raw[i];
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

    // Find similar pulses and average them
    size_t newCount = 0;
    unsigned long lastPulse = signalBuffer.smooth[0];
    int similarCount = 1;

    for (size_t i = 1; i < signalBuffer.smoothCount; i++)
    {
        if (abs((long)(signalBuffer.smooth[i] - lastPulse)) < signalBuffer.errorTolerance)
        {
            lastPulse = (lastPulse * similarCount + signalBuffer.smooth[i]) / (similarCount + 1);
            similarCount++;
        }
        else
        {
            signalBuffer.smooth[newCount++] = lastPulse;
            lastPulse = signalBuffer.smooth[i];
            similarCount = 1;
        }
    }

    // Store the last pulse group
    if (similarCount > 0)
    {
        signalBuffer.smooth[newCount++] = lastPulse;
    }

    signalBuffer.smoothCount = newCount;
    return true;
}

bool RFSignalProcessor::transmitRaw(const long *timings, size_t count, int repetitions)
{
    if (!timings || count == 0 || count > MAX_SAMPLES)
    {
        log_e("Invalid transmission parameters");
        return false;
    }

    // Check memory state before transmission
    size_t freeHeap = MEM_MGR.getFreeHeap();
    if (freeHeap < 10240)
    { // 10KB minimum
        log_w("Low memory before transmission: %u bytes", freeHeap);
        MEM_MGR.defragment();
    }

    ELECHOUSE_cc1101.SetTx();

    for (int rep = 0; rep < repetitions; rep++)
    {
        for (size_t i = 0; i < count && timings[i] != 0; i += 2)
        {
            digitalWrite(TXPin0, HIGH);
            delayMicroseconds(timings[i]);
            digitalWrite(TXPin0, LOW);
            delayMicroseconds(timings[i + 1]);
        }
        delay(10); // Small delay between repetitions
    }

    ELECHOUSE_cc1101.SetRx();
    return true;
}

bool RFSignalProcessor::decodeBinary()
{
    // This is a placeholder for implementing binary signal decoding
    // The actual implementation would analyze timing patterns to extract binary data
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

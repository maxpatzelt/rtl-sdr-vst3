/*
  AM Demodulator Implementation
*/

#include "AMDemodulator.h"
#include <algorithm>

AMDemodulator::AMDemodulator()
{
}

AMDemodulator::~AMDemodulator()
{
}

void AMDemodulator::setSampleRate(double rate)
{
    sampleRate = rate;
}

float AMDemodulator::iqToFloat(uint8_t val) const
{
    // Convert uint8 (0-255) to float (-1.0 to +1.0) - optimized
    constexpr float scale = 1.0f / 127.5f;
    return (static_cast<float>(val) - 127.5f) * scale;
}

float AMDemodulator::calculateEnvelope(float I, float Q)
{
    // Envelope detection: sqrt(I² + Q²)
    return std::sqrt(I * I + Q * Q);
}

float AMDemodulator::removeDC(float sample)
{
    if (!dcBlockEnabled)
        return sample;

    // High-pass filter to remove DC offset
    // y[n] = x[n] - dcOffset
    // dcOffset = dcOffset * alpha + x[n] * (1 - alpha)

    float output = sample - dcOffset;
    dcOffset = dcOffset * dcAlpha + sample * (1.0f - dcAlpha);

    return output;
}

float AMDemodulator::applyAGC(float sample)
{
    if (!agcEnabled)
        return sample;

    // Simple AGC: measure signal level and adjust gain
    float absSample = std::abs(sample);

    // Calculate error from target level
    float error = agcTarget - absSample;

    // Adjust gain based on error
    agcGain += error * agcSpeed;

    // Limit gain range
    agcGain = std::clamp(agcGain, 0.1f, 10.0f);

    return sample * agcGain;
}

float AMDemodulator::process(uint8_t I, uint8_t Q)
{
    // Convert IQ to float
    float If = iqToFloat(I);
    float Qf = iqToFloat(Q);

    // Calculate envelope (AM signal strength)
    float envelope = calculateEnvelope(If, Qf);

    // Remove DC component
    float dcRemoved = removeDC(envelope);

    // Apply AGC
    float output = applyAGC(dcRemoved);

    return output;
}

void AMDemodulator::processBlock(const uint8_t* iqData, int numIQBytes, std::vector<float>& output)
{
    // Process IQ pairs (numIQBytes should be even: I,Q,I,Q,...)
    int numSamples = numIQBytes / 2;
    output.reserve(output.size() + numSamples);

    for (int i = 0; i < numIQBytes; i += 2)
    {
        uint8_t I = iqData[i];
        uint8_t Q = iqData[i + 1];

        output.push_back(process(I, Q));
    }
}

/*
  SSB Demodulator Implementation
*/

#include "SSBDemodulator.h"
#include <algorithm>

SSBDemodulator::SSBDemodulator()
{
    initializeHilbertFilter();
    initializeBandPassFilter();
}

SSBDemodulator::~SSBDemodulator()
{
}

void SSBDemodulator::setSampleRate(double rate)
{
    sampleRate = rate;
    initializeBandPassFilter();
}

float SSBDemodulator::iqToFloat(uint8_t val) const
{
    // Optimized conversion
    constexpr float scale = 1.0f / 127.5f;
    return (static_cast<float>(val) - 127.5f) * scale;
}

void SSBDemodulator::initializeHilbertFilter()
{
    // Simple Hilbert transform approximation
    // This creates a 90-degree phase shift
    // For proper SSB, you'd want a longer FIR filter

    const int filterLength = 65;
    hilbertCoefficients.resize(filterLength);

    int center = filterLength / 2;
    for (int i = 0; i < filterLength; ++i)
    {
        int n = i - center;
        if (n == 0)
        {
            hilbertCoefficients[i] = 0.0f;
        }
        else if (n % 2 == 0)
        {
            hilbertCoefficients[i] = 0.0f;
        }
        else
        {
            hilbertCoefficients[i] = 2.0f / (juce::MathConstants<float>::pi * n);
        }
    }

    // Apply window
    for (int i = 0; i < filterLength; ++i)
    {
        float window = 0.54f - 0.46f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (filterLength - 1));
        hilbertCoefficients[i] *= window;
    }
}

void SSBDemodulator::initializeBandPassFilter()
{
    // Band-pass filter for SSB audio (300 Hz - 2700 Hz typical)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;

    bpFilter.prepare(spec);

    // Simple band-pass using IIR coefficients
    // Low-pass at 2700 Hz, high-pass at 300 Hz
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 1500.0, 1000.0);
    *bpFilter.coefficients = *coeffs;
}

float SSBDemodulator::processSSB(float I, float Q)
{
    // SSB demodulation using phasing method
    // USB: I*cos + Q*sin (I - Q_hilbert)
    // LSB: I*cos - Q*sin (I + Q_hilbert)

    // For simplicity, we'll use I+Q for USB, I-Q for LSB
    // A proper implementation would use Hilbert transform

    float output;
    if (ssbMode == USB)
    {
        output = I - Q;  // Upper sideband
    }
    else
    {
        output = I + Q;  // Lower sideband
    }

    return output;
}

float SSBDemodulator::removeDC(float sample)
{
    float output = sample - dcOffset;
    dcOffset = dcOffset * dcAlpha + sample * (1.0f - dcAlpha);
    return output;
}

float SSBDemodulator::applyAGC(float sample)
{
    if (!agcEnabled)
        return sample;

    float absSample = std::abs(sample);
    float error = agcTarget - absSample;
    agcGain += error * agcSpeed;
    agcGain = std::clamp(agcGain, 0.1f, 20.0f);

    return sample * agcGain;
}

void SSBDemodulator::processBlock(const uint8_t* iqData, int numIQBytes, std::vector<float>& output)
{
    int numSamples = numIQBytes / 2;
    output.reserve(output.size() + numSamples);

    for (int i = 0; i < numIQBytes; i += 2)
    {
        uint8_t Iu8 = iqData[i];
        uint8_t Qu8 = iqData[i + 1];

        float I = iqToFloat(Iu8);
        float Q = iqToFloat(Qu8);

        // SSB detection
        float demod = processSSB(I, Q);

        // Remove DC
        demod = removeDC(demod);

        // Apply AGC
        demod = applyAGC(demod);

        output.push_back(demod);
    }
}

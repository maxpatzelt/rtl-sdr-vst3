/*
  FM Demodulator Implementation
*/

#include "FMDemodulator.h"
#include <cmath>

FMDemodulator::FMDemodulator()
{
    reset();
}

FMDemodulator::~FMDemodulator()
{
}

void FMDemodulator::setSampleRate(double sdrSampleRate)
{
    sdrRate = sdrSampleRate;

    // Update low-pass filter for channel selection
    // Cutoff at bandwidth/2
    float cutoff = bandwidth / 2.0f;
    float normalizedCutoff = cutoff / (float)sdrRate;

    // Use JUCE IIR filter (Butterworth low-pass)
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sdrRate, cutoff, 0.707f);
}

void FMDemodulator::setAudioSampleRate(double audioRate_)
{
    audioRate = audioRate_;
}

void FMDemodulator::setBandwidth(float bandwidthHz)
{
    bandwidth = bandwidthHz;
    setSampleRate(sdrRate); // Recalculate filter
}

void FMDemodulator::setDeemphasis(float timeConstantUs)
{
    // De-emphasis filter: first-order IIR low-pass
    // Time constant: 50μs (Europe) or 75μs (US)
    // alpha = 1 / (1 + RC * fs) where RC = time constant

    float rc = timeConstantUs * 1e-6f; // Convert to seconds
    deemphasisAlpha = 1.0f / (1.0f + rc * (float)audioRate);
}

void FMDemodulator::setSquelch(float threshold)
{
    squelchThreshold = threshold;
}

void FMDemodulator::reset()
{
    prevIQ = std::complex<float>(1.0f, 0.0f);
    deemphasisState = 0.0f;

    // Initialize filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sdrRate;
    spec.maximumBlockSize = 16384;
    spec.numChannels = 1;
    lowPassFilter.prepare(spec);

    setSampleRate(sdrRate);
    setDeemphasis(75.0f); // US standard
}

void FMDemodulator::iqToComplex(const uint8_t* iq, int numSamples, std::vector<std::complex<float>>& out)
{
    // RTL-SDR gives uint8 IQ pairs centered at 127
    // Convert to float complex centered at 0 (optimized)
    int complexSamples = numSamples / 2;
    out.resize(complexSamples);

    // Optimized conversion
    constexpr float scale = 1.0f / 128.0f;
    for (int i = 0; i < complexSamples; ++i)
    {
        int idx = i * 2;
        float I = (static_cast<float>(iq[idx]) - 127.5f) * scale;
        float Q = (static_cast<float>(iq[idx + 1]) - 127.5f) * scale;
        out[i] = std::complex<float>(I, Q);
    }
}

void FMDemodulator::demodulate(const std::vector<std::complex<float>>& iq, std::vector<float>& audio)
{
    audio.resize(iq.size());

    // Polar discriminator (phase difference method)
    // This is the classic approach from rtl_fm
    // demod = angle(conj(prev) * current)

    float sumMagnitude = 0.0f;

    for (size_t i = 0; i < iq.size(); i++)
    {
        std::complex<float> current = iq[i];

        // Track signal level for squelch
        float magnitude = std::abs(current);
        sumMagnitude += magnitude;

        // Multiply by conjugate of previous sample
        std::complex<float> diff = std::conj(prevIQ) * current;

        // Extract phase (FM signal is in the phase)
        float angle = std::arg(diff);

        audio[i] = angle;
        prevIQ = current;
    }

    // Update signal level (exponential moving average)
    if (!iq.empty())
    {
        float avgMagnitude = sumMagnitude / iq.size();
        signalLevel = signalLevel * (1.0f - signalLevelAlpha) + avgMagnitude * signalLevelAlpha;
    }

    // Apply squelch: mute if signal level below threshold
    if (squelchThreshold > 0.0f && signalLevel < squelchThreshold)
    {
        std::fill(audio.begin(), audio.end(), 0.0f);
    }
}

void FMDemodulator::applyDeemphasis(std::vector<float>& audio)
{
    // Simple first-order IIR: y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
    for (size_t i = 0; i < audio.size(); i++)
    {
        deemphasisState = deemphasisAlpha * audio[i] + (1.0f - deemphasisAlpha) * deemphasisState;
        audio[i] = deemphasisState;
    }
}

void FMDemodulator::processIQ(const uint8_t* iqData, int numSamples, std::vector<float>& audioOut)
{
    if (numSamples == 0)
        return;

    // Step 1: Convert IQ to complex
    std::vector<std::complex<float>> complexIQ;
    iqToComplex(iqData, numSamples, complexIQ);

    // Step 2: Optional - Apply low-pass filter for channel selection
    // (Skipping for now to keep it simple - can add later)

    // Step 3: FM demodulation
    std::vector<float> demodulated;
    demodulate(complexIQ, demodulated);

    // Step 4: De-emphasis
    applyDeemphasis(demodulated);

    // Step 5: Output
    audioOut = std::move(demodulated);
}

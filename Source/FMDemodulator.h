/*
  FM Demodulator
  Implements wideband FM demodulation from IQ samples

  DSP architecture inspired by:
  - rtl_fm: Classic FM demodulation approach
  - rtl_fm_player: Audio pipeline structure
  - SDRPlusPlus FM demodulator: Modern filter design

  Algorithm:
  1. Convert uint8 IQ to float complex
  2. Low-pass filter for channel selection
  3. Polar discriminator for FM demodulation
  4. De-emphasis filter (50μs or 75μs)
  5. Output audio samples
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <complex>

class FMDemodulator
{
public:
    FMDemodulator();
    ~FMDemodulator();

    // Configuration
    void setSampleRate(double sdrSampleRate);
    void setAudioSampleRate(double audioRate);
    void setBandwidth(float bandwidthHz); // Channel bandwidth
    void setDeemphasis(float timeConstantUs); // 50 or 75 microseconds
    void setSquelch(float threshold); // 0.0 to 1.0 (0 = off)

    // Process IQ samples (uint8 format from RTL-SDR)
    // Returns demodulated audio samples
    void processIQ(const uint8_t* iqData, int numSamples, std::vector<float>& audioOut);

    // Reset state
    void reset();

private:
    // IQ to complex conversion (RTL-SDR uses uint8 centered at 127)
    void iqToComplex(const uint8_t* iq, int numSamples, std::vector<std::complex<float>>& out);

    // FM demodulation using polar discriminator
    // Method from rtl_fm: phase difference between consecutive samples
    void demodulate(const std::vector<std::complex<float>>& iq, std::vector<float>& audio);

    // Apply de-emphasis filter
    void applyDeemphasis(std::vector<float>& audio);

    double sdrRate = 1024000.0;
    double audioRate = 48000.0;
    float bandwidth = 150000.0f; // 150 kHz for wideband FM

    // Squelch
    float squelchThreshold = 0.0f;
    float signalLevel = 0.0f;
    float signalLevelAlpha = 0.1f;

    // De-emphasis filter state (simple first-order IIR)
    float deemphasisAlpha = 0.0f;
    float deemphasisState = 0.0f;

    // Previous IQ sample for discriminator
    std::complex<float> prevIQ{1.0f, 0.0f};

    // Low-pass filter using JUCE DSP
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                     juce::dsp::IIR::Coefficients<float>> lowPassFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMDemodulator)
};

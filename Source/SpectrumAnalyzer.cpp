/*
  Spectrum Analyzer Implementation
*/

#include "SpectrumAnalyzer.h"
#include <cmath>

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    fftData.fill(0.0f);
    magnitudes.fill(minDb);
    smoothedMags.fill(minDb);

    startTimerHz(20);  // 20 FPS refresh (optimized)
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    stopTimer();
}

void SpectrumAnalyzer::pushSamples(const uint8_t* iqData, int numSamples)
{
    if (!iqData || numSamples == 0) return;

    // Convert IQ pairs to complex float
    for (int i = 0; i < numSamples && i < fftSize; i += 2)
    {
        if (writePosition >= fftSize) break;

        // IQ data comes as [I0, Q0, I1, Q1, ...]
        float I = iqToFloat(iqData[i]);
        float Q = iqToFloat(iqData[i + 1]);

        fftData[writePosition * 2] = I;      // Real
        fftData[writePosition * 2 + 1] = Q;  // Imaginary

        writePosition++;
    }

    // When buffer is full, perform FFT
    if (writePosition >= fftSize)
    {
        performFFT();
        writePosition = 0;
        needsUpdate = true;
    }
}

float SpectrumAnalyzer::iqToFloat(uint8_t val) const
{
    // Convert uint8 (0-255) to float (-1.0 to +1.0) - optimized
    constexpr float scale = 1.0f / 127.5f;
    return (static_cast<float>(val) - 127.5f) * scale;
}

void SpectrumAnalyzer::performFFT()
{
    // Apply windowing function to reduce spectral leakage
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate magnitudes in dB
    for (int i = 0; i < fftSize / 2; ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);

        // Convert to dB
        float db = magnitude > 0.0f ? 20.0f * std::log10(magnitude) : minDb;
        magnitudes[i] = juce::jlimit(minDb, maxDb, db);
    }
}

void SpectrumAnalyzer::timerCallback()
{
    if (needsUpdate)
    {
        // Smooth the magnitudes for stable display
        for (int i = 0; i < fftSize / 2; ++i)
        {
            smoothedMags[i] = smoothedMags[i] * smoothingFactor +
                             magnitudes[i] * (1.0f - smoothingFactor);
        }

        needsUpdate = false;
        repaint();
    }
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xFF1a1a1a));

    // Grid
    g.setColour(juce::Colour(0xFF2a2a2a));
    for (int i = 0; i < 10; ++i)
    {
        float y = bounds.getHeight() * i / 10.0f;
        g.drawHorizontalLine((int)y, 0.0f, (float)bounds.getWidth());
    }

    // Frequency labels
    g.setColour(juce::Colours::grey);
    g.setFont(10.0f);

    double span = (sampleRate / 1e6) / zoomLevel;  // MHz (apply zoom)
    double startFreq = centerFrequency - span / 2.0;
    double endFreq = centerFrequency + span / 2.0;

    g.drawText(juce::String(startFreq, 2) + " MHz", 5, bounds.getHeight() - 15, 60, 12, juce::Justification::left);
    g.drawText(juce::String(endFreq, 2) + " MHz", bounds.getWidth() - 65, bounds.getHeight() - 15, 60, 12, juce::Justification::right);

    // Spectrum curve
    juce::Path spectrumPath;
    bool started = false;

    for (int i = 0; i < fftSize / 2; ++i)
    {
        float x = juce::jmap((float)i, 0.0f, (float)(fftSize / 2), 0.0f, (float)bounds.getWidth());
        float db = smoothedMags[i];
        float normalised = juce::jmap(db, minDb, maxDb, 1.0f, 0.0f);
        float y = bounds.getHeight() * normalised;

        if (!started)
        {
            spectrumPath.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            spectrumPath.lineTo(x, y);
        }
    }

    // Fill under curve with gradient
    juce::Path fillPath = spectrumPath;
    fillPath.lineTo((float)bounds.getWidth(), (float)bounds.getHeight());
    fillPath.lineTo(0.0f, (float)bounds.getHeight());
    fillPath.closeSubPath();

    juce::ColourGradient gradient(
        juce::Colour(0xFF00ff00).withAlpha(0.3f), 0.0f, 0.0f,
        juce::Colour(0xFF00ff00).withAlpha(0.0f), 0.0f, (float)bounds.getHeight(),
        false);

    g.setGradientFill(gradient);
    g.fillPath(fillPath);

    // Draw spectrum line
    g.setColour(juce::Colour(0xFF00ff00));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));

    // Center frequency marker
    g.setColour(juce::Colours::red.withAlpha(0.5f));
    float centerX = bounds.getWidth() / 2.0f;
    g.drawVerticalLine((int)centerX, 0.0f, (float)bounds.getHeight());
}

void SpectrumAnalyzer::resized()
{
    // Nothing to resize
}

void SpectrumAnalyzer::mouseDown(const juce::MouseEvent& event)
{
    if (!onFrequencyClicked)
        return;

    auto bounds = getLocalBounds();
    float clickX = (float)event.getPosition().getX();
    float relativeX = clickX / bounds.getWidth();

    // Convert pixel position to frequency
    // relativeX: 0.0 = left edge, 0.5 = center, 1.0 = right edge
    double span = (sampleRate / 1e6) / zoomLevel;  // MHz (apply zoom)
    double clickedFreq = centerFrequency + (relativeX - 0.5) * span;

    // Clamp to valid range
    clickedFreq = juce::jlimit(24.0, 1766.0, clickedFreq);

    // Notify callback
    onFrequencyClicked(clickedFreq);
}

void SpectrumAnalyzer::exportScreenshot(const juce::File& outputFile)
{
    // Create an image of the current display
    juce::Image screenshot(juce::Image::RGB, getWidth(), getHeight(), true);
    juce::Graphics g(screenshot);

    // Render the spectrum analyzer to the image
    paint(g);

    // Save as PNG
    juce::FileOutputStream stream(outputFile);
    if (stream.openedOk())
    {
        juce::PNGImageFormat pngWriter;
        pngWriter.writeImageToStream(screenshot, stream);
    }
}

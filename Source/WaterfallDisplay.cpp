/*
  Waterfall Display Implementation
*/

#include "WaterfallDisplay.h"
#include <cmath>

WaterfallDisplay::WaterfallDisplay()
    : fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann),
      history(historyLines)
{
    fftData.fill(0.0f);
    for (auto& line : history)
        line.fill(-100.0f);

    waterfallImage = juce::Image(juce::Image::RGB, fftSize / 2, historyLines, true);

    startTimerHz(15);  // 15 FPS refresh (optimized)
}

WaterfallDisplay::~WaterfallDisplay()
{
    stopTimer();
}

void WaterfallDisplay::pushSamples(const uint8_t* iqData, int numSamples)
{
    if (!iqData || numSamples == 0) return;

    for (int i = 0; i < numSamples && i < fftSize; i += 2)
    {
        if (writePosition >= fftSize) break;

        float I = iqToFloat(iqData[i]);
        float Q = iqToFloat(iqData[i + 1]);

        fftData[writePosition * 2] = I;
        fftData[writePosition * 2 + 1] = Q;

        writePosition++;
    }

    if (writePosition >= fftSize)
    {
        performFFT();
        writePosition = 0;
    }
}

float WaterfallDisplay::iqToFloat(uint8_t val) const
{
    return (static_cast<float>(val) - 127.5f) / 127.5f;
}

void WaterfallDisplay::performFFT()
{
    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Store this line in history
    currentLine = (currentLine + 1) % historyLines;

    for (int i = 0; i < fftSize / 2; ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);

        float db = magnitude > 0.0f ? 20.0f * std::log10(magnitude) : -100.0f;
        history[currentLine][i] = juce::jlimit(-100.0f, 0.0f, db);
    }

    updateWaterfall();
}

void WaterfallDisplay::updateWaterfall()
{
    // Update waterfall image
    for (int y = 0; y < historyLines; ++y)
    {
        int lineIdx = (currentLine - y + historyLines) % historyLines;

        for (int x = 0; x < fftSize / 2; ++x)
        {
            float db = history[lineIdx][x];
            float normalized = juce::jmap(db, -100.0f, 0.0f, 0.0f, 1.0f);

            // Color mapping: blue (weak) -> green -> yellow -> red (strong)
            juce::Colour color;
            if (normalized < 0.25f)
            {
                color = juce::Colour::fromHSV(0.67f, juce::jmap(normalized, 0.0f, 0.25f, 0.0f, 1.0f), normalized * 4.0f, 1.0f);
            }
            else if (normalized < 0.5f)
            {
                color = juce::Colour::fromHSV(0.33f, 1.0f, juce::jmap(normalized, 0.25f, 0.5f, 0.3f, 1.0f), 1.0f);
            }
            else if (normalized < 0.75f)
            {
                color = juce::Colour::fromHSV(0.16f, 1.0f, juce::jmap(normalized, 0.5f, 0.75f, 0.5f, 1.0f), 1.0f);
            }
            else
            {
                color = juce::Colour::fromHSV(0.0f, 1.0f, normalized, 1.0f);
            }

            waterfallImage.setPixelAt(x, y, color);
        }
    }
}

void WaterfallDisplay::timerCallback()
{
    repaint();
}

void WaterfallDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xFF0a0a0a));

    // Draw waterfall image scaled to component size
    g.drawImage(waterfallImage, bounds.toFloat());

    // Overlay grid
    g.setColour(juce::Colour(0xFF2a2a2a));
    for (int i = 1; i < 10; ++i)
    {
        float x = bounds.getWidth() * i / 10.0f;
        g.drawVerticalLine((int)x, 0.0f, (float)bounds.getHeight());
    }

    // Center frequency marker
    g.setColour(juce::Colours::red.withAlpha(0.3f));
    float centerX = bounds.getWidth() / 2.0f;
    g.drawVerticalLine((int)centerX, 0.0f, (float)bounds.getHeight());

    // Frequency labels
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(9.0f);

    double span = sampleRate / 1e6;
    double startFreq = centerFrequency - span / 2.0;
    double endFreq = centerFrequency + span / 2.0;

    g.drawText(juce::String(startFreq, 2) + " MHz", 2, 2, 60, 12, juce::Justification::left);
    g.drawText(juce::String(endFreq, 2) + " MHz", bounds.getWidth() - 62, 2, 60, 12, juce::Justification::right);
}

void WaterfallDisplay::resized()
{
    // Nothing to resize
}

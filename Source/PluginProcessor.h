/*
  RTL-SDR VST3 Plugin Processor
  Bridges RTL-SDR hardware to Ableton Live audio

  Architecture:
  1. RTL-SDR thread: Capture IQ samples → Ring buffer
  2. Audio thread: Ring buffer → Demodulation → Resampling → Audio output
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RTLSDRDevice.h"
#include "FMDemodulator.h"
#include "AMDemodulator.h"
#include "SSBDemodulator.h"
#include "AudioResampler.h"
#include "RingBuffer.h"

class RTLSDRPluginProcessor : public juce::AudioProcessor
{
public:
    RTLSDRPluginProcessor();
    ~RTLSDRPluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // RTL-SDR Control
    bool startSDR(int deviceIndex = 0);
    void stopSDR();
    bool isSDRRunning() const { return sdrDevice.isStreaming(); }

    std::vector<RTLSDRDevice::DeviceInfo> getAvailableDevices();

    // Expose IQ buffer for visualization
    RingBuffer<uint8_t>& getIQBuffer() { return iqBuffer; }

    // Parameters (using smart pointers for proper ownership)
    juce::AudioParameterFloat* frequencyParam = nullptr;
    juce::AudioParameterFloat* gainParam = nullptr;
    juce::AudioParameterChoice* modeParam = nullptr;
    juce::AudioParameterFloat* volumeParam = nullptr;
    juce::AudioParameterFloat* fineTuneParam = nullptr;
    juce::AudioParameterFloat* squelchParam = nullptr;

private:
    // SDR components
    RTLSDRDevice sdrDevice;
    FMDemodulator fmDemod;
    AMDemodulator amDemod;
    SSBDemodulator ssbDemod;
    AudioResampler resampler;
    RingBuffer<uint8_t> iqBuffer;    // IQ samples from SDR
    RingBuffer<float> audioBuffer;    // Demodulated audio

    // Parameters
    std::atomic<float> currentFrequency{100.0f}; // MHz
    std::atomic<float> currentGain{29.0f};       // dB
    std::atomic<float> currentVolume{0.7f};
    std::atomic<int> currentMode{0};             // 0=FM WB, 1=FM NB, 2=AM, 3=USB, 4=LSB

    // Audio state
    double hostSampleRate = 48000.0;

    // Processing buffers
    std::vector<uint8_t> tempIQBuffer;
    std::vector<float> tempAudioBuffer;

    // SDR callback
    void onSDRSamples(const uint8_t* data, uint32_t length);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RTLSDRPluginProcessor)
};

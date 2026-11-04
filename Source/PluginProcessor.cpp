/*
  RTL-SDR Plugin Processor Implementation
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

RTLSDRPluginProcessor::RTLSDRPluginProcessor()
     : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), false)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Add parameters using proper ownership
    addParameter(frequencyParam = new juce::AudioParameterFloat(
        juce::ParameterID{"frequency", 1}, "Frequency (MHz)",
        juce::NormalisableRange<float>(24.0f, 1766.0f, 0.001f),
        100.0f));

    addParameter(gainParam = new juce::AudioParameterFloat(
        juce::ParameterID{"gain", 1}, "Gain (dB)",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        29.0f));

    addParameter(modeParam = new juce::AudioParameterChoice(
        juce::ParameterID{"mode", 1}, "Mode",
        juce::StringArray{"FM (Wideband)", "FM (Narrowband)", "AM", "USB", "LSB"},
        0));

    addParameter(volumeParam = new juce::AudioParameterFloat(
        juce::ParameterID{"volume", 1}, "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f));

    addParameter(fineTuneParam = new juce::AudioParameterFloat(
        juce::ParameterID{"finetune", 1}, "Fine Tune",
        juce::NormalisableRange<float>(-10000.0f, 10000.0f, 10.0f),
        0.0f));

    addParameter(squelchParam = new juce::AudioParameterFloat(
        juce::ParameterID{"squelch", 1}, "Squelch",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));
}

RTLSDRPluginProcessor::~RTLSDRPluginProcessor()
{
    stopSDR();
}

const juce::String RTLSDRPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RTLSDRPluginProcessor::acceptsMidi() const
{
    return true;  // Enable MIDI for CC control
}

bool RTLSDRPluginProcessor::producesMidi() const
{
    return false;
}

bool RTLSDRPluginProcessor::isMidiEffect() const
{
    return false;
}

double RTLSDRPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RTLSDRPluginProcessor::getNumPrograms()
{
    return 1;
}

int RTLSDRPluginProcessor::getCurrentProgram()
{
    return 0;
}

void RTLSDRPluginProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String RTLSDRPluginProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void RTLSDRPluginProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void RTLSDRPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;

    // Configure demodulator and resampler
    fmDemod.setSampleRate(1024000.0); // 1.024 MHz SDR rate
    fmDemod.setAudioSampleRate(sampleRate);
    fmDemod.setBandwidth(150000.0f); // 150 kHz for wideband FM

    resampler.setRates(1024000.0, sampleRate);

    // Clear buffers
    iqBuffer.clear();
    audioBuffer.clear();
}

void RTLSDRPluginProcessor::releaseResources()
{
    stopSDR();
}

bool RTLSDRPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono or stereo output
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo())
        return true;

    return false;
}

void RTLSDRPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Process MIDI CC messages for parameter control
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isController())
        {
            int ccNumber = message.getControllerNumber();
            float ccValue = message.getControllerValue() / 127.0f;

            // MIDI CC mappings (standard):
            // CC 1 (Mod Wheel) = Frequency
            // CC 7 (Volume) = Volume
            // CC 74 (Brightness) = Gain
            // CC 71 (Resonance) = Squelch
            if (ccNumber == 1 && frequencyParam)  // Frequency
                frequencyParam->setValueNotifyingHost(ccValue);
            else if (ccNumber == 7 && volumeParam)  // Volume
                volumeParam->setValueNotifyingHost(ccValue);
            else if (ccNumber == 74 && gainParam)  // Gain
                gainParam->setValueNotifyingHost(ccValue);
            else if (ccNumber == 71 && squelchParam)  // Squelch
                squelchParam->setValueNotifyingHost(ccValue);
        }
    }

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear input channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update parameters
    float freqMHz = frequencyParam->get();
    if (std::abs(freqMHz - currentFrequency.load()) > 0.001f)
    {
        currentFrequency.store(freqMHz);
        sdrDevice.setFrequency((uint32_t)(freqMHz * 1e6f));
    }

    float gainDb = gainParam->get();
    if (std::abs(gainDb - currentGain.load()) > 0.1f)
    {
        currentGain.store(gainDb);
        sdrDevice.setGain((int)(gainDb * 10.0f)); // Convert to tenths of dB
    }

    currentVolume.store(volumeParam->get());
    currentMode.store(modeParam->getIndex());

    // Process SDR samples
    int samplesNeeded = buffer.getNumSamples();

    // Read IQ samples from buffer and demodulate
    size_t iqAvailable = iqBuffer.getReadAvailable();

    if (iqAvailable > 0)
    {
        // Read IQ samples
        size_t toRead = std::min(iqAvailable, (size_t)32768);
        tempIQBuffer.resize(toRead);
        iqBuffer.read(tempIQBuffer.data(), toRead);

        // Demodulate based on mode
        tempAudioBuffer.clear();
        int mode = currentMode.load();

        if (mode == 2)  // AM mode
        {
            amDemod.processBlock(tempIQBuffer.data(), (int)toRead, tempAudioBuffer);
        }
        else if (mode == 3)  // USB mode
        {
            ssbDemod.setMode(SSBDemodulator::USB);
            if (fineTuneParam)
                ssbDemod.setFineTuneOffset(fineTuneParam->get());
            ssbDemod.processBlock(tempIQBuffer.data(), (int)toRead, tempAudioBuffer);
        }
        else if (mode == 4)  // LSB mode
        {
            ssbDemod.setMode(SSBDemodulator::LSB);
            if (fineTuneParam)
                ssbDemod.setFineTuneOffset(fineTuneParam->get());
            ssbDemod.processBlock(tempIQBuffer.data(), (int)toRead, tempAudioBuffer);
        }
        else  // FM modes (0 = Wideband, 1 = Narrowband)
        {
            // Set bandwidth based on mode
            if (mode == 0)
                fmDemod.setBandwidth(150000.0f);  // Wideband FM: 150 kHz
            else if (mode == 1)
                fmDemod.setBandwidth(12500.0f);   // Narrowband FM: 12.5 kHz (NOAA weather)

            // Set squelch threshold
            if (squelchParam)
                fmDemod.setSquelch(squelchParam->get());

            fmDemod.processIQ(tempIQBuffer.data(), (int)toRead, tempAudioBuffer);
        }

        // Resample to host rate
        std::vector<float> resampledAudio;
        resampler.process(tempAudioBuffer, resampledAudio);

        // Write to audio buffer
        audioBuffer.write(resampledAudio.data(), resampledAudio.size());
    }

    // Fill output buffer
    size_t audioAvailable = audioBuffer.getReadAvailable();

    if (audioAvailable >= (size_t)samplesNeeded)
    {
        std::vector<float> outputSamples(samplesNeeded);
        audioBuffer.read(outputSamples.data(), samplesNeeded);

        // Apply volume and copy to all output channels
        float volume = currentVolume.load();

        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int i = 0; i < samplesNeeded; ++i)
            {
                channelData[i] = outputSamples[i] * volume;
            }
        }
    }
    else
    {
        // Not enough audio - output silence
        buffer.clear();
    }
}

bool RTLSDRPluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* RTLSDRPluginProcessor::createEditor()
{
    return new RTLSDRPluginEditor(*this);
}

void RTLSDRPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameters
    juce::MemoryOutputStream stream(destData, true);
    stream.writeFloat(frequencyParam->get());
    stream.writeFloat(gainParam->get());
    stream.writeInt(modeParam->getIndex());
    stream.writeFloat(volumeParam->get());
}

void RTLSDRPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameters
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);

    if (stream.getNumBytesRemaining() >= 4)
        frequencyParam->setValueNotifyingHost(frequencyParam->getNormalisableRange().convertTo0to1(stream.readFloat()));

    if (stream.getNumBytesRemaining() >= 4)
        gainParam->setValueNotifyingHost(gainParam->getNormalisableRange().convertTo0to1(stream.readFloat()));

    if (stream.getNumBytesRemaining() >= 4)
        modeParam->setValueNotifyingHost((float)stream.readInt() / (modeParam->choices.size() - 1));

    if (stream.getNumBytesRemaining() >= 4)
        volumeParam->setValueNotifyingHost(volumeParam->getNormalisableRange().convertTo0to1(stream.readFloat()));
}

bool RTLSDRPluginProcessor::startSDR(int deviceIndex)
{
    if (!sdrDevice.openDevice(deviceIndex))
        return false;

    // Configure SDR
    sdrDevice.setFrequency((uint32_t)(currentFrequency.load() * 1e6f));
    sdrDevice.setSampleRate(1024000); // 1.024 MHz
    sdrDevice.setAutoGain(false);
    sdrDevice.setGain((int)(currentGain.load() * 10.0f));

    // Start streaming with callback
    return sdrDevice.startStreaming([this](const uint8_t* data, uint32_t length) {
        onSDRSamples(data, length);
    });
}

void RTLSDRPluginProcessor::stopSDR()
{
    sdrDevice.stopStreaming();
    sdrDevice.closeDevice();
}

std::vector<RTLSDRDevice::DeviceInfo> RTLSDRPluginProcessor::getAvailableDevices()
{
    return sdrDevice.enumerateDevices();
}

void RTLSDRPluginProcessor::onSDRSamples(const uint8_t* data, uint32_t length)
{
    // Called from SDR thread - just buffer the samples
    iqBuffer.write(data, length);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RTLSDRPluginProcessor();
}

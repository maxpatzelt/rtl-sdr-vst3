/*
  RTL-SDR Plugin Editor (GUI)
  Professional SDR interface with spectrum analyzer, waterfall, and presets
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"
#include "WaterfallDisplay.h"
#include "SignalMeter.h"
#include "PresetManager.h"
#include "FrequencyHistory.h"

class RTLSDRPluginEditor : public juce::AudioProcessorEditor,
                           public juce::Timer
{
public:
    RTLSDRPluginEditor(RTLSDRPluginProcessor&);
    ~RTLSDRPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    RTLSDRPluginProcessor& audioProcessor;

    // Visualization components
    SpectrumAnalyzer spectrumAnalyzer;
    WaterfallDisplay waterfallDisplay;
    SignalMeter signalMeter;

    // Preset management
    PresetManager presetManager;

    // Frequency history
    FrequencyHistory frequencyHistory;
    juce::ComboBox historyCombo;

    // UI Components
    juce::Label titleLabel;
    juce::Label statusLabel;

    juce::Label frequencyLabel;
    juce::Slider frequencySlider;

    juce::Label gainLabel;
    juce::Slider gainSlider;

    juce::Label modeLabel;
    juce::ComboBox modeCombo;

    juce::Label volumeLabel;
    juce::Slider volumeSlider;

    juce::Label fineTuneLabel;
    juce::Slider fineTuneSlider;

    juce::Label squelchLabel;
    juce::Slider squelchSlider;

    juce::ComboBox deviceCombo;
    juce::TextButton startButton;
    juce::TextButton stopButton;

    // Visualization controls
    juce::TextButton zoomInButton;
    juce::TextButton zoomOutButton;
    juce::TextButton exportButton;

    // Preset UI
    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton;
    juce::TextButton deletePresetButton;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    void updateDeviceList();
    void updateStatus();
    void updatePresetList();
    void loadPreset(int index);
    void saveCurrentAsPreset();
    void deleteCurrentPreset();
    void updateHistoryList();
    void addToHistory(double frequency);

    float currentZoom = 1.0f;  // Track spectrum zoom level

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RTLSDRPluginEditor)
};

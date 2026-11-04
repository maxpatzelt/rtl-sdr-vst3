# RTL-SDR VST3 Plugin for Ableton Live

A professional-grade VST3 plugin that bridges RTL-SDR USB radio dongles to Ableton Live, featuring real-time spectrum visualization, waterfall display, and comprehensive preset management for radio frequencies.

![RTL-SDR Radio Plugin UI](screenshots/plugin-ui.png)

## 🎯 Features

### Core SDR Features
- **Real-time RF to Audio**: Stream radio signals directly into Ableton Live
- **Multi-Mode Demodulation**: FM (Wide/Narrow), AM, SSB (USB/LSB)
- **Frequency Range**: 24 MHz to 1.76 GHz (RTL-SDR hardware dependent)
- **MIDI Control**: Full MIDI CC support for DAW automation
- **Low Latency**: Optimized for real-time audio performance (< 100ms)

### Professional Visualizations
- **FFT Spectrum Analyzer**: Real-time 1024-point FFT with zoom (0.1x-10x)
- **Waterfall Display**: Time-domain spectrum with color-mapped intensity
- **S-Meter**: Professional signal strength (dBm and S-units)
- **Click-to-Tune**: Interactive spectrum for instant frequency selection
- **Screenshot Export**: Save spectrum views as PNG images

### Advanced Controls
- **Fine Tuning**: ±10 kHz adjustment for SSB precision
- **Squelch**: Noise gate for FM modes (0.0-1.0)
- **Frequency History**: 20-entry recent frequency memory
- **Preset Manager**: Save/load station configurations
- **Zoom Controls**: Spectrum analyzer frequency span adjustment

### MIDI Integration
- **CC 1 (Mod Wheel)**: Frequency control
- **CC 7 (Volume)**: Output volume
- **CC 74 (Brightness)**: Receiver gain
- **CC 71 (Resonance)**: Squelch threshold
- Full parameter automation support in any DAW

## 🏗️ Architecture

This plugin implements a complete SDR-to-DAW pipeline with real-time visualization:

```
RTL-SDR Hardware
    ↓ (USB)
IQ Sample Stream (uint8, 1.024 MSPS)
    ├→ (Visualization: FFT + Waterfall + S-Meter)
    ↓ (Thread-safe ring buffer)
FM Demodulator (Polar discriminator)
    ↓ (De-emphasis filter)
Audio Resampler (1.024 MHz → 48 kHz)
    ↓ (Ring buffer)
VST3 Audio Output → Ableton Live
```

### Component Overview

1. **RTLSDRDevice** (`RTLSDRDevice.cpp`)
   - Device enumeration and initialization
   - Frequency tuning and gain control
   - IQ sample streaming via librtlsdr
   - Inspired by: librtlsdr examples, rtl_fm_player

2. **FMDemodulator** (`FMDemodulator.cpp`)
   - Polar discriminator for FM demodulation
   - Low-pass filtering for channel selection
   - De-emphasis filter (50μs/75μs)
   - Based on: rtl_fm, SDRPlusPlus FM demod

3. **AudioResampler** (`AudioResampler.cpp`)
   - High-quality sample rate conversion
   - Lagrange interpolation
   - Pattern from: JUCE interpolators, SDR++ resampling

4. **SpectrumAnalyzer** (`SpectrumAnalyzer.cpp`)
   - 1024-point FFT with Hann windowing
   - Smoothed magnitude display in dB
   - Real-time frequency grid and labels

5. **WaterfallDisplay** (`WaterfallDisplay.cpp`)
   - 200-line history buffer
   - HSV color mapping for signal intensity
   - Blue (weak) → Green → Yellow → Red (strong)

6. **SignalMeter** (`SignalMeter.cpp`)
   - RMS power calculation from IQ samples
   - S-unit conversion (S0-S9 + dB over S9)
   - Color-coded meter bar

7. **PresetManager** (`PresetManager.cpp`)
   - XML-based preset storage
   - Default preset library with 17+ stations
   - Add/remove/update preset functionality

4. **RingBuffer** (`RingBuffer.h`)
   - Lock-free circular buffer
   - Thread-safe producer-consumer
   - Adapted from: SDRPlusPlus ring buffer, JUCE AbstractFifo

5. **PluginProcessor** (`PluginProcessor.cpp`)
   - VST3 audio processing
   - Parameter management
   - Thread coordination

## 🔧 Dependencies

### Required

- **JUCE Framework** (v7.0+): https://github.com/juce-framework/JUCE
- **CMake** (v3.15+)
- **C++17 Compiler** (MSVC 2019+, Clang, GCC)

### Platform-Specific

#### Windows
- Visual Studio 2019 or later
- RTL-SDR driver: https://github.com/osmocom/rtl-sdr/releases
- librtlsdr (zadig for driver installation)

#### macOS
- Xcode 12 or later
- Homebrew: `brew install librtlsdr cmake`

## 📦 Building

### 1. Clone and Setup

```bash
git clone --recurse-submodules https://github.com/maxpatzelt/rtl-sdr-vst3.git RTL-SDR-VST
cd RTL-SDR-VST

# Initialize JUCE if this repository was cloned without submodules
git submodule update --init --recursive
```

### 2. Install RTL-SDR Library

#### Windows
1. Download prebuilt librtlsdr from https://github.com/osmocom/rtl-sdr/releases
2. Extract to `C:\Program Files\rtl-sdr`
3. Add to PATH or set `RTLSDR_LIBRARY` and `RTLSDR_INCLUDE_DIR` in CMake

#### macOS
```bash
brew install librtlsdr
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get install librtlsdr-dev
```

### 3. Build with CMake

```bash
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# The VST3 will be in: build/RTLSDRPlugin_artefacts/Release/VST3/
```

### 4. Install Plugin

#### Windows
Copy to: `C:\Program Files\Common Files\VST3\`

#### macOS
Copy to: `~/Library/Audio/Plug-Ins/VST3/` or `/Library/Audio/Plug-Ins/VST3/`

## 🎵 Usage in Ableton Live

1. **Load the Plugin**
   - Create an audio track in Ableton Live
   - Add "RTL-SDR Radio" from the Audio Effects
   - Or: Use on a MIDI track if generating audio from scratch

2. **Connect RTL-SDR Hardware**
   - Plug in your RTL-SDR dongle
   - Select device from dropdown in plugin GUI
   - Click "Start SDR"

3. **Tune to a Frequency**
   - Use the frequency slider or preset buttons
   - FM Radio: 88-108 MHz
   - Aviation: 118-137 MHz
   - Weather Radio: 162-163 MHz

4. **Adjust Settings**
   - **Gain**: Increase for weak signals, decrease if distorted
   - **Mode**: FM Wideband (default), FM Narrowband, AM
   - **Volume**: Output level to DAW

5. **Process with Ableton**
   - Apply audio effects (reverb, delay, distortion)
   - Record the audio
   - Use with Max for Live devices
   - Sample and resample RF signals

## 🔬 Open Source Inspiration

This project draws architectural patterns and DSP techniques from:

- **librtlsdr**: Device control, tuning, sample streaming
  - https://github.com/osmocom/rtl-sdr

- **rtl_fm / rtl_fm_player**: FM demodulation pipeline
  - https://github.com/AlbrechtL/rtl_fm_player

- **SDRPlusPlus**: Modular DSP architecture, filters, resampling
  - https://github.com/AlexandreRouma/SDRPlusPlus

- **rtl-ais**: Signal processing structure
  - https://github.com/dgiardini/rtl-ais

- **rtl-sdr-scanner-cpp**: C++ device management patterns
  - https://github.com/shajen/rtl-sdr-scanner-cpp

No code was copied verbatim—only architectural patterns, DSP algorithms, and best practices were adapted.

## 📝 Technical Notes

### Sample Rate
- RTL-SDR runs at 1.024 MHz (configurable in code)
- Audio output at host DAW rate (typically 44.1/48 kHz)
- Resampling uses Lagrange interpolation

### Thread Model
- **SDR Thread**: Captures IQ samples from hardware
- **Audio Thread**: Demodulates and outputs to DAW
- **Ring Buffers**: Thread-safe communication

### Demodulation
- **FM**: Polar discriminator (phase difference)
- **De-emphasis**: 75μs filter (US standard)
- **Bandwidth**: 150 kHz for wideband FM

### Latency
- Typical latency: 50-100ms (depends on buffer sizes)
- Configurable via audio buffer settings

## 🐛 Troubleshooting

### No devices found
- Ensure RTL-SDR dongle is connected
- Install librtlsdr drivers (zadig on Windows)
- Check device with: `rtl_test` command

### No audio output
- Verify "Start SDR" is clicked
- Check frequency is valid for your region
- Increase gain if signal is weak
- Ensure Ableton track is armed/monitoring

### Crackling/distortion
- Reduce gain
- Check USB connection quality
- Close other USB-intensive applications

### Build errors
- Verify JUCE submodule is initialized
- Check CMake finds librtlsdr
- Ensure C++17 compiler is used

## 🚀 Future Enhancements

Potential features for future development:

- [ ] AM/SSB/CW demodulation modes
- [ ] Spectrum analyzer visualization
- [ ] Waterfall display
- [ ] Automatic gain control (AGC)
- [ ] Frequency scanning
- [ ] Recording to file
- [ ] Multiple SDR device support
- [ ] IQ file playback mode

## 📄 License

This project is released under the MIT License; see [LICENSE](LICENSE). JUCE
and librtlsdr are third-party dependencies and retain their own license terms.

### Third-Party Licenses
- **JUCE**: GPL v3 / Commercial (https://juce.com/juce-7-licence/)
- **librtlsdr**: GPL v2 (https://github.com/osmocom/rtl-sdr)

## 🤝 Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test thoroughly with real hardware
4. Submit a pull request

## 📧 Contact

For questions, issues, or suggestions, please open a GitHub issue.

---

**Disclaimer**: Use responsibly and in compliance with local radio regulations. Ensure you have the right to receive and process radio signals in your jurisdiction.

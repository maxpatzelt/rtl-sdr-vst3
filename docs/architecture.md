# SDR-VST Architecture

## Pipeline
The plugin connects RTL-SDR IQ capture to host audio output. Hardware capture,
demodulation, sample-rate conversion, visualization, and VST3 hosting are kept
as separate responsibilities so the audio callback can remain focused.

## Components
- `Source/` contains the JUCE processor/editor and radio/DSP components.
- `JUCE/` is pinned as a Git submodule at the commit recorded by SDR-VST.
- `screenshots/` stores project UI reference assets.
- `docs/` stores project-level architecture and maintenance notes.
- `build/` is generated output and is not versioned.

## Boundaries
- Keep unrelated assistant, tunnel, and container configuration local and out of the plugin repository.
- Keep changes to JUCE explicit as submodule pointer updates; do not vendor a second JUCE copy.
- Retain third-party license notices alongside the project license.

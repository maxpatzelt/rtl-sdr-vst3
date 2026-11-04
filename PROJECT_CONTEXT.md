# SDR-VST Project Context

Updated: 2026-11-03.

## Purpose
- Develop a VST3 plugin that receives RTL-SDR radio input and routes demodulated audio into a DAW.
- Maintain DSP, hardware integration, spectrum visualization, and plugin UI in one project.

## Repository
- NAS working path: `VSCODE PROGRAMS/Audio Programming/vst-plugins/SDR-VST/`.
- GitHub: `https://github.com/maxpatzelt/rtl-sdr-vst3`.
- Primary branch: `master`.
- `JUCE/` is a Git submodule pinned by the parent repository; `.gitmodules` records its official upstream URL.

## Structure
- `Source/` contains the plugin processor, UI, and signal processing.
- `JUCE/` contains the pinned JUCE dependency.
- `screenshots/` contains UI reference images.
- `docs/` contains project architecture, decisions, and changelog.
- `build/` contains generated local output and is ignored by Git.
- Root-level `chat-frank*`, `frank*`, `daemon.json`, and `docker-compose-ollama.yml` files are local operational tooling, not plugin project content; they remain on the NAS and are ignored here.

## Build Requirements
- CMake 3.15 or later.
- A C++17 compiler.
- JUCE and the platform's librtlsdr development library.
- Build and hardware/runtime validation have not been performed as part of this repository setup.

## Working Rules
- Keep plugin source, build configuration, screenshots, and relevant documentation in Git.
- Keep generated builds, local credentials, and unrelated operational tooling out of Git.
- Preserve the JUCE submodule commit unless intentionally upgrading the dependency.
- Follow the project and third-party license terms in `LICENSE`.

# SDR-VST Decisions

## Repository and Dependency Boundaries
- Keep `rtl-sdr-vst3` as the canonical repository for this plugin.
- Track JUCE as the existing `JUCE/` submodule using its official upstream URL.
- Keep generated `build/` output out of Git.
- Keep local assistant/tunnel/container configuration out of this project.

## Licensing
- The project license is MIT as recorded in `LICENSE`.
- JUCE and librtlsdr retain their own license terms; review them before distributing builds.

## Validation
- Record build, host, and hardware verification only after those checks have been run.
- The current source changes are not build-verified in this setup session.

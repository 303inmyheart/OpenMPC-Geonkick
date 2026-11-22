# Limits and workarounds

- Gen2/aarch64 and MPC OS 3.10.1+. Hardware testing is limited to Live III.
- Stereo output, 16 shared voices. Original kit choke groups, mute/solo and separate outputs are not implemented. Use MPC's own humanize/randomization features.
- WAV import uses the first channel, resampled to44.1kHz and bounded to the first four seconds. Embedded kit PCM is bounded to11MiB and VST chunks to16MiB. Invalid files leave the assigned sound intact.
- Original factory curves and source identities remain until their corresponding controls are edited. The native point editor supports at most16 knots. This is a native MPC port, not the original desktop GUI or complete upstream feature parity; see the [engine audit](ENGINE-PARITY.md).
- The native Edit preset dropdown is a flat VST program list. Factory-pack folders need a supported host/API route; slash-prefixed names do not supply native categories.
- On the maintainer's MPC, tapping an already-selected external-plugin thumbnail under Sounds → Instruments does not reopen its preset list. This also occurs with other external VSTs. Use the bottom preset arrows or the native in-editor preset chooser; investigation is deferred.
- A standard benchmark PASS does not imply that the additional full-parameter sweep passes or that every project is glitch-free. Background synthesis is measured separately; its percentage of one CPU core is not the MPC project CPU meter. All exact [reports](../benchmarks) are retained.

Further work and deferred features: [roadmap](../ROADMAP.md).

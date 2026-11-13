# Geonkick MPC roadmap

Target: Gen2 / aarch64, MPC OS 3.10.1+. Hardware tests: none yet.

1. **0.0.1: first playable prototype** — original DSP isolated; nine oscillators,
   three layers, global filter/drive, eight starting sounds, eight-voice playback,
   native pages/Q-Links, MPC program bank, Sounds tile and project state.
2. **Device validation** — installer, pad notes/velocity, layer/oscillator
   subpages, wheel and Q-Link synchronization, preset browser, Sounds tile,
   project save/reload, multiple instances. Measure live worker CPU during real
   time parameter edits, as well as the official audio-thread benchmark.
3. **Sound editing parity** — original arbitrary and Bezier envelopes, per-osc
   effects, full useful filter range, sample loading, external preset import,
   waveform display and preview/export. Evaluate native envelope editing first.
4. **Kit architecture** — sixteen independent drums, pad/key/channel mapping,
   per-drum choke/mute/solo, kit persistence, practical navigation. Decide output
   routing within the MPC VST host's actual capabilities; do not promise desktop
   multichannel parity before verification.
5. **Release** — visual refinement, public fork and source review, reproducible
   build workflow, exact hardware-tested release, benchmark evidence, catalog PR.

The remaining JC303 feature-parity items stay deferred. Following targets:
Fire, Wolf Shaper, BYOD navigation feasibility review, then Surge.

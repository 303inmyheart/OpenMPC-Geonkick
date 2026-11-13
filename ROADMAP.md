# Geonkick MPC roadmap

Target: Gen2 / aarch64, MPC OS 3.10.1+. Hardware tests: none yet.

1. **0.0.1: first playable prototype** — original DSP isolated; nine oscillators,
   three layers, global filter/drive, eight starting sounds, eight-voice playback,
   native pages/Q-Links, MPC program bank, Sounds tile and project state.
2. **Device validation** — installer, pad notes/velocity, layer/oscillator
   subpages, wheel and Q-Link synchronization, preset browser, Sounds tile,
   project save/reload, multiple instances. Measure live worker CPU during real
   time parameter edits, as well as the official audio-thread benchmark.
   - **0.0.2 implemented offline:** independent original filter and drive per
     oscillator, amp attack controls, nine FX subpages, 184 append-only
     parameters and migration of 0.0.1 project chunks. Native sanitizers and
     ARM64 shared-library/engine tests pass. No hardware claim yet.
   - **0.0.3 implemented offline:** visual SYNTH page with actual completed-hit
     waveform, amplitude/pitch envelope, editing controls and three oscillator
     modules. Native FilmStrip displays, UI-thread notifications, old-state
     migration, gray/green layout and actual-DSP offline preview. Device UI
     verification is the next gate; free point/Bezier editing remains pending.
   - **0.0.4:** 432 two-pixel waveform columns, finer vertical levels and corrected
     native text/captions. Approved update and app restart completed; on-screen
     plugin review is pending because MPC restarted into the track-type selector.
3. **Sound editing parity** — original arbitrary and Bezier envelopes,
   full useful filter range, sample loading, external preset import,
   graphical point editing and preview/export. Evaluate native envelope editing first.
4. **Kit architecture** — sixteen independent drums, pad/key/channel mapping,
   per-drum choke/mute/solo, kit persistence, practical navigation. Decide output
   routing within the MPC VST host's actual capabilities; do not promise desktop
   multichannel parity before verification.
5. **Release** — visual refinement, public fork and source review, reproducible
   build workflow, exact hardware-tested release, benchmark evidence, catalog PR.

The remaining JC303 feature-parity items stay deferred. Following targets:
Fire, Wolf Shaper, BYOD navigation feasibility review, then Surge.

# OpenMPC Geonkick

The MPC adapter, build scripts and interface are AI-generated. Hardware testing
of this new port has **not yet taken place**. The original Geonkick DSP is the
work of Iurie Nistor and contributors, not AI-generated.

Development port of [Geonkick 3.9.0](https://github.com/quamplex/geonkick) for
Gen2 MPCs (aarch64), targeting MPC OS 3.10.1+. Version 0.0.1 is a first playable
single-instrument prototype, not the complete desktop application.

Implemented:

- Three synthesis layers, three oscillators per layer; independent oscillator
  enable, waveform, frequency, level, phase, noise density and decay.
- Sine, square, triangle, saw, white noise and Brownian noise.
- Pitch-drop amount and time; first-oscillator FM into the second per layer,
  matching the original engine's routing.
- Global low/high/band-pass filter and nine original distortion modes.
- Eight voices, velocity, optional note tracking, tuning, poly/cut/gate playback.
- Eight starting sounds: Init, Deep Kick, Tight Kick, Snare, Closed Hat,
  Open Hat, Metal and Tom. Native VST program bank for MPC's preset browser.
- Native MPC pages and Q-Links, per-layer oscillator subpages, Sounds-browser
  artwork and an Init .xpl. Frames include complete label and value bounds.
- Versioned project-state serialization covering all 103 parameters.
- Per-instance worker generates sound buffers; audio playback neither allocates
  nor waits for synthesis. Existing hits retain their old sound during edits.

Editing a sound is asynchronous: new notes use the latest **completed** sound.
During rapid changes they may briefly use the preceding one. Output, tuning,
note tracking and playback mode apply at playback; layers/filter/oscillators are
rendered into the sound. Layer 1 is audible by default; layers 2 and 3 start at
zero gain. On the PRESETS page, RESET INIT restores Init even if Init is already
the selected preset; selecting the same preset value alone does not reload it.

First-prototype limits: one drum per instance and stereo output, no 16-drum kit
editor, sample import, arbitrary graphical envelope editor, per-oscillator
filter/distortion UI or humanizer. Amp and pitch envelopes are generated from
the decay and pitch-time controls. Pink noise is omitted because the pinned
upstream implementation returns zero. The original state-variable filter is
restricted to 30–8000 Hz and resonance 10–1000 to keep its discrete-time update
stable at the MPC engine's 44.1 kHz. This needs revisiting for full parity.

See [LICENSE-AUDIT.md](LICENSE-AUDIT.md), [BUILDING.md](BUILDING.md), [TESTING.md](TESTING.md),
[ROADMAP.md](ROADMAP.md) and [source provenance](src/VENDORED.md).
Installer ZIPs are local development artifacts; no public GitHub repository,
release or store listing has been created for this port yet.

License: **GPL-3.0-or-later**, including vendored Geonkick and Quamplex DSP Tools.
The external OpenMPC wrapper is MIT; the bundled DejaVu font has its own notice
in art/FONT-LICENSE.txt. No Akai stock artwork or proprietary SDK is included.

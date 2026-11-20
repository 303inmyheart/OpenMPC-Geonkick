# Filter models — Geonkick MPC0.0.19

MASTER → FX and each LAYER → FX page have an independent FILTER MODEL selector.
It is touch-only; the four rotary controls keep their Q-Links and complete frames.

- **WIDE:**20 Hz–20 kHz LP/HP/BP, with a trapezoidal state-variable design.
  Resonance is shown as a percentage plus actual Q, mapped logarithmically from
  Q0.5 to20. Default Q1 corresponds to18.8%. Band-pass centre gain is normalized;
  high LP/HP resonance intentionally raises the peak and can reach the existing
  output limiter. Adjust layer/oscillator/output levels to taste.
- **CLASSIC:** the unchanged original Geonkick recurrence, with30–8000 Hz cutoff
  and original resonance factor10–1000. The displayed percentage maps that old
  factor logarithmically. Sweeping past its limits returns the actual accepted
  position to the native Touch control, including while stopped.

Q-Links/data wheel use5-cent cutoff steps and0.1-percent resonance steps. Touch
uses continuous direct normalized coordinates; cutoff is logarithmic, so equal
finger travel covers equal frequency ratios. Both routes address one physical
value. Merely rereading/echoing a parameter does not round a saved frequency.

Both models retain their own frequency and resonance values. New Init sounds
start WIDE. GKMP1–5 projects migrate all ten filter models to CLASSIC to retain
their original sound; corresponding WIDE values are initialized from the older
frequency and bounded equivalent Q. The original two hat presets remain CLASSIC.
Old parameter IDs/ranges/defaults are frozen; new physical values/proxies are
appended and saved in GKMP6. Older plugin builds cannot read a new GKMP6 project
state, so keep the previous project file if testing a newer development build.

Each parameter edit generates a new completed hit on the existing bounded
worker. Already sounding notes keep their original cached sound; subsequent
notes use completed new sounds. Cut/end fades and quiet-time waveform refresh
are unchanged. No new synthesis wait, allocation, mutex or UI callback runs on
MPC's audio thread. Coefficients are cached instead of evaluating tan per sample
for the currently constant filter envelopes.

Tests cover independent bilinear frequency-response arithmetic, the complete
range/Q/type grid, old CLASSIC arithmetic, real VST Touch/Q-Link events, state
migration and continuity of held notes. Thirty actual0.0.18 old-project fixtures
compare bit-for-bit with the current CLASSIC path. These are offline native and
ARM64/QEMU functional checks; they are not a Store CPU benchmark or hearing test.

Mathematical reference: [Andrew Simper, Cytomic: trapezoidal state-variable filter](https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf).
The port's C implementation is original code under GPL-3.0-or-later; no external
implementation library or proprietary source is bundled.

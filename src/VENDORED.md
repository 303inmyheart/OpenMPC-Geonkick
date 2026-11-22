# Geonkick DSP source

Copied from https://github.com/quamplex/geonkick at
`462fed9bd8c48579a8819e7b360f8a2999b246fc` (Geonkick 3.9.0),
directory `src/dsp/src`, including its Quamplex DSP Tools headers.
Copyright Iurie Nistor and contributors; GPL-3.0-or-later. See LICENSE
and original per-file notices. Port changes are documented below; the original per-file license notices remain.

The MPC adapter uses the oscillator, envelope, filter, distortion and synthesizer
code directly. It replaces desktop hosting, the global worker and playback with
per-instance asynchronous synthesis and a bounded voice/cache pool. No Redkite,
JACK, LV2, VST3 SDK, RapidJSON or libsndfile is linked into this prototype.
Pink noise is not offered: upstream's implementation currently returns zero.

Local adapters, build scripts and UI are separate from the vendored files.
The OpenMPC wrapper remains an external MIT-licensed build dependency.


Sample integration adds non-owning worker-only views of the original sample
buffer. Buffers now initialize port-only fade/pitch-cache fields to disabled;
borrowed views enable a bounded source-end fade and cache exactly unchanged
semitone factors. The adapter detaches borrowed buffers before upstream teardown.
This leaves ordinary oscillators and owned buffers on their prior paths. No
libsndfile dependency or external audio assets have been added.

0.0.24 adds optional per-layer taps to the existing synthesizer sample loop.
Oscillators/FM/filter state are advanced once; original mixed synthesis remains
the audio path. The worker reduces these taps into cached waveform peaks, before
shared master filter/drive, with no per-tab re-synthesis.

0.0.35 retains the envelope equations and float results while using `fabsf`
for differences already computed as float. The DBL_EPSILON threshold is retained.
Finite constant two-point envelopes within normalized0–1 bounds return their
exact zero-slope result after the original endpoint checks. Degenerate and
extreme spans keep the original interpolation path. A frozen0.0.34 evaluator
and more than416,000 bit-exact comparisons cover linear/Bezier/table curves,
knots, subnormals, signed zero and invalid/extreme inputs. Original licence
notices remain in both implementation and frozen test reference.

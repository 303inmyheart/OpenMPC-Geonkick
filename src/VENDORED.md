# Geonkick DSP source

Copied from https://github.com/quamplex/geonkick at
`462fed9bd8c48579a8819e7b360f8a2999b246fc` (Geonkick 3.9.0),
directory `src/dsp/src`, including its Quamplex DSP Tools headers.
Copyright Iurie Nistor and contributors; GPL-3.0-or-later. See LICENSE
and original per-file notices. The original C files are initially unmodified.

The MPC adapter uses the oscillator, envelope, filter, distortion and synthesizer
code directly. It replaces desktop hosting, the global worker and playback with
per-instance asynchronous synthesis and a bounded voice/cache pool. No Redkite,
JACK, LV2, VST3 SDK, RapidJSON or libsndfile is linked into this prototype.
Pink noise is not offered: upstream's implementation currently returns zero.

Local adapters, build scripts and UI are separate from the vendored files.
The OpenMPC wrapper remains an external MIT-licensed build dependency.

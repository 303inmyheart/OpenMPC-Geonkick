# Original-engine audit — after0.0.21

Compared with the pinned Geonkick3.9.0 source, not a claim of desktop GUI parity.

| Area | MPC state |
|---|---|
| Three layers / nine oscillators | Implemented independently for16 pad sounds |
| Original tone/noise, FM, phase, layer/oscillator gains | Implemented; upstream pink-noise stub omitted |
| Amp/Pitch envelopes | Simple +2–16 cubic points, full waveform overlay, XY editing |
| Filter modes | Original CLASSIC + port-specific stable WIDE, global +nine oscillator paths |
| Distortion | Original nine modes, global +nine oscillator paths |
| Humanizer |Removed0.0.21 at user request; seven inert compatibility IDs |
| Filter cutoff/Q envelopes | Original DSP supports them; MPC currently uses constant envelopes |
| Drive envelope | Original DSP supports it; not exposed as a point-editable MPC curve |
| Noise-density / pitch-shift envelopes | Original DSP types exist; MPC currently exposes simpler controls |
| Fully free Bezier handle positions / tap-to-insert | Deferred UI extensions |
| Sample loading, original preset/bank interchange, export | Next phase after pad foundation; not implemented |
| Sixteen-drum kits / pad and routing architecture | 16 sounds, selection, MIDI mapping and full state implemented0.0.21; routing/management extensions deferred |

The user skipped additional filter/resonance and other modulation-envelope UI
work. Do not restart it automatically. Next: sample loading per selected pad,
then.gkick presets/.gkit soundbanks and subsequent kit-management features.

Original source: https://github.com/quamplex/geonkick (pinned source/provenance in
src/VENDORED.md). Humanizer comparison uses HumanizerView.cpp and dsp/humanizer.c
already present in the reviewed upstream checkout; no new licence is introduced.

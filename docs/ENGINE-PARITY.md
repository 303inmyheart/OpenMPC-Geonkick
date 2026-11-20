# Original-engine audit — after0.0.20

Compared with the pinned Geonkick3.9.0 source, not a claim of desktop GUI parity.

| Area | MPC state |
|---|---|
| Three layers / nine oscillators | Implemented for one percussion instrument |
| Original tone/noise, FM, phase, layer/oscillator gains | Implemented; upstream pink-noise stub omitted |
| Amp/Pitch envelopes | Simple +2–16 cubic points, full waveform overlay, XY editing |
| Filter modes | Original CLASSIC + port-specific stable WIDE, global +nine oscillator paths |
| Distortion | Original nine modes, global +nine oscillator paths |
| Humanizer |0.0.20 velocity/timing variation, optional/default OFF |
| Filter cutoff/Q envelopes | Original DSP supports them; MPC currently uses constant envelopes |
| Drive envelope | Original DSP supports it; not exposed as a point-editable MPC curve |
| Noise-density / pitch-shift envelopes | Original DSP types exist; MPC currently exposes simpler controls |
| Fully free Bezier handle positions / tap-to-insert | Deferred UI extensions |
| Sample loading, original preset/bank interchange, export | Phase4, not implemented |
| Sixteen-drum kits / pad and routing architecture | Phase5, not implemented |

Next useful engine increment: reuse the existing point infrastructure for filter
cutoff/resonance, preserving old curve/state identities and keeping waveform
feedback bounded. Other modulation curves can follow in separate reviewable
builds. Samples/presets remain the next larger phase after useful engine parity.

Original source: https://github.com/quamplex/geonkick (pinned source/provenance in
src/VENDORED.md). Humanizer comparison uses HumanizerView.cpp and dsp/humanizer.c
already present in the reviewed upstream checkout; no new licence is introduced.

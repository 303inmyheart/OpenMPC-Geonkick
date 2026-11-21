# Original-engine audit — after0.0.23

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
| Sample loading, original preset/bank interchange, export | WAV import per pad/oscillator0.0.23; native preset/bank interchange and export follow |
| Sixteen-drum kits / pad and routing architecture | 16 sounds, selection, MIDI mapping and full state implemented0.0.21; routing/management extensions deferred |

The user skipped additional filter/resonance and other modulation-envelope UI
work. Do not restart it automatically. WAV loading is implemented. Next:.gkick presets/.gkit soundbanks via native MPC presets/Sounds and subsequent kit-management features.

Original source: https://github.com/quamplex/geonkick (pinned source/provenance in
src/VENDORED.md). Humanizer comparison uses HumanizerView.cpp and dsp/humanizer.c
already present in the reviewed upstream checkout; no new licence is introduced.


0.0.29 original factory work: master Amp and original raw Amp/Pitch/noise/filter/Q/
drive/volume curves are retained internally for imported factory sounds, together
with original seeds/FM/sample pitch. The skipped general modulation-envelope UI
is still skipped. Existing controls edit canonical targets; the native Amp/Pitch
editor remains limited to16 knots after explicit curve editing. All102 original
individual sounds and7 kits are converted; arbitrary user-file import remains
separate. Frozen VST identities/ranges remain unchanged; imported scalars outside
those ranges are preserved until corresponding control edits. Factory kit outputs
are folded to stereo; general choke/mute/solo/multi-output work remains deferred.

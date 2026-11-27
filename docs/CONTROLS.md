# Geonkick MPC controls

## Navigation

- **SOUND:** waveform overview, layer selection and the most useful tone,
  amplitude and pitch controls. Select LAYER 1, 2 or 3 to edit that layer.
- **L1, L2, L3:** each layer has OSC 1–3 and FX 1–3 pages. Each OSC owns a
  waveform/noise/sample source, frequency, envelope and level. Its matching FX
  page controls that oscillator's filter and drive. Enable those effects first.
- **ENVELOPE:** amplitude/pitch curves and touch-only point editing. The
  waveform follows the selected oscillator/layer; point editing has no Q-Links.
- **MASTER:** KIT, SAMPLES, MIX and MASTER FX.
- **PRESET INFO:** English help for older personal presets. Tap outside to close.

Q-Links follow visible rotary controls. Switches, source selection, point editing
and action buttons use touch. Q-Link/data-wheel frequency changes are fine
(5 cents per accumulated step); touch changes are continuous and coarser.

## Getting a first kick

Start from Init: layer 1 / oscillator 1 enabled, SINE, about 55 Hz, short Attack,
Decay around 300 ms, Pitch Drop around 24 semitones and Pitch Time around 40 ms.
Layer 2 and 3 can be silent until their Layer Level is raised. Frequency and pitch
sweep do not affect white/brown noise in the same way as a pitched waveform;
noise sources expose Noise Density instead.

The updated sound is rendered in the background. New hits use the latest ready
sound; an already-playing hit keeps its existing audio. LENGTH sets the maximum
rendered duration. OUTPUT controls the whole plugin's output level.

## Pads and kits

Under MASTER → KIT choose **16 PADS**. If the MPC pad-note layout differs, stop
playback, select the desired MPC bank, enable **LEARN 16**, then press pads 01–16
in order. Save the kit as a native MPC preset to keep the learned mapping.
INIT PAD resets the selected sound; loading native Init resets the whole plugin.
Sixteen independent sounds share 16 playback voices and stereo output.

## Native presets and samples

The native MPC browser/program menu offers 118 starting states, including 109
original factory sounds/kits. Loading a complete preset replaces the whole
plugin state, including all pads and MIDI mapping. Save changes with MPC's own
preset management; imported samples are embedded in that state.

MASTER → SAMPLES has tap-to-preview, Stop, Load and Clear actions. Loading is
separate from previewing. Select the target oscillator before Load. WAV import
uses the first channel and first four seconds, resampled to 44.1 kHz. A remembered
sample-folder setting is preserved on plugin upgrade. See [sample guide](SAMPLES.md).

## Older personal presets

A manufacturer change can require a new XPL header even when the sound data are
valid. Use the [computer converter](PRESET-CONVERTER.md) to create copies, then
load them on the MPC. Originals and encoded sound data stay unchanged. The plugin
installer does not run the converter automatically.

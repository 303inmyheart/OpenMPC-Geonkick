# Geonkick MPC

**The MPC adapter, interface and build scripts are 100% AI-generated and tested by the maintainer on MPC Live III.** Original Geonkick DSP, factory sounds, font and OpenMPC framework retain their authorship and licences.

Percussion synthesis and sampling for **Gen2 MPCs, MPC OS 3.10.1+**. Based on [Geonkick 3.9.0](https://github.com/quamplex/geonkick). Hardware tested on **Live III only**.

**[Download 1.0](https://github.com/303inmyheart/OpenMPC-Geonkick/releases/tag/geonkick-mpc-v1.0.0)** · [Screenshots](docs/SCREENSHOTS.md) · [Controls](BEDIENUNG.md) · [Roadmap](ROADMAP.md)

![Geonkick MPC on MPC Live III](docs/screenshots/synth.png)

## Features

- **16 independent pad sounds**, each with three layers and nine oscillators; synth waves, noise, FM and WAV samples.
- **118 native MPC programs**: eight starter sounds, a 16-pad starter kit and 109 original factory presets/kits.
- **Amplitude and pitch envelopes** with 2–16 editable points, waveform overlay and segment curvature.
- Oscillator/master filters and drive; Poly, Cut and Gate playback; learned hardware-pad mapping.
- **Tap-to-preview sample browser**, remembered folder and separate Load action. Samples are embedded in saved kits.
- Fine Q-Link/data-wheel input, continuous touchscreen controls and complete kit/project recall. Q-Links control rotary knobs; point editing is touch-only.

## Install

Give the `Geonkick-MPC-1.0.0-mpc-aarch64.zip` release asset directly to the **OpenMPC Installer or Plugin Manager**. Enable **External Plugin Support** in MPC Preferences → Enhancements. Installation restarts the MPC application. Gen2/aarch64 only; no Gen1 build.

For hardware pads, use MASTER → KIT → **16 PADS**. If the note layout differs, stop playback, select the desired MPC bank, enable **LEARN 16** and press pads 01–16 in order. Relearn after changing the MPC note layout.

## Performance and limits

The standard OpenMPC benchmark passes on Live III. The additional full-parameter stress test still fails; background synthesis can use a substantial part of one CPU core. See the exact [release measurements](benchmarks/1.0.0) and [test scope](TESTING.md).

Stereo output and 16 shared voices. WAV import uses the first channel and first four seconds, resampled to 44.1 kHz. Native preset folders, broader kit management and original user `.gkick`/`.gkit` import remain future work. [Limits and workarounds](docs/LIMITS.md).

## Source and licences

Fork of [quamplex/geonkick](https://github.com/quamplex/geonkick). Software: **GPL-3.0-or-later**; external OpenMPC wrapper: MIT; font: [DejaVu notice](art/FONT-LICENSE.txt). [Build instructions](BUILDING.md) and [licence review](LICENSE-AUDIT.md).

Factory sounds retain their separate **CC0, CC BY 4.0, CC BY-SA 4.0 or CC BY-SA 3.0** terms. Original credits, licences and conversion notices are preserved in [factory/NOTICE.txt](factory/NOTICE.txt) and the installer. This community port is not an official Akai/inMusic or Geonkick release.

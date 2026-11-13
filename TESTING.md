# Test status — 2026-11-13

Offline only:

- Native actual VST shared-library host under ASan/UBSan with leak detection:
  two instances, parameter reads/writes, option stepping and Q-Link sweeps,
  state round trip, eight VST programs, MIDI controls and concurrent host access.
- Independent sine/chirp/amplitude/fade oracle across 127-frame chunks:
  maximum error 3.9e-7 native and 2.6e-7 ARM64.
- Full-length note-off behavior, gate note/channel matching, all-notes-off,
  edited state across layers, rejection of malformed/non-finite input, audible
  starting sounds, instance isolation and rapid cache changes with active voices.
- ARM64 actual shared object loaded by the host under QEMU, plus the same
  behavioural suite compiled for ARM64. No emulator timing is a CPU claim.
- 0.0.2: all nine individual oscillator filter paths change captured audio;
  with each oscillator soloed, its drive output at zero produces silence.
  Attack softens the initial hit. These tests pass natively and on ARM64 under QEMU.
- 0.0.1-shaped 103-value chunks load with the 81 appended controls at defaults;
  0.0.2 states round-trip edited values across the original and appended fields.
- Native pages rendered offline and visually inspected. Q-Link bounds include
  complete controls. All 21 generated subpages exist; no popup overlays or
  skin-check overlap/edge warnings.
- Official release.py development installer generated; catalog_check.py with
  --catalog --require-portable passes with zero warnings. ZIP checksums and
  mandatory installer files verified. This checks package format, not public
  availability of the future source repository or real-device behavior.

Logs and byte-specific test receipts are local under build/. No MPC installation
or host interaction has taken place. The configured Live III was unreachable
over SSH during this work, so no device benchmark is available.

Pending maintainer checks on Live III / MPC OS 3.10.1:

| Check | Result |
|---|---|
| OpenMPC Installer, insert instrument, hear default Init | Pending |
| Nine oscillator pages, all three layer gains | Pending |
| Wheel/Q-Links match switches and waveform selections | Pending |
| Native preset browser, eight programs, Sounds tile | Pending |
| Save/reload edited project; second independent instance | Pending |
| Actual audio and synthesis-worker CPU, rapid edits, long session | Pending |

The formal fast benchmark is insufficient for this architecture's background
synthesis load: it advances audio faster than wall clock. Worker CPU must also
be sampled during paced playback and editing. Do not publish a PASS claim from
the fast benchmark alone.

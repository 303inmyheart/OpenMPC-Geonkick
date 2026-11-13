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
  complete controls. All 24 generated subpages exist; no popup overlays or
  skin-check overlap/edge warnings.
- 0.0.3: waveform extrema match captured audio within the 64-level display
  quantization; amplitude and pitch overlays match the envelope; graph values
  reject host writes. Completed graphs can refresh with no audio render calls.
  GKMP1 and GKMP2 chunks migrate into the new GKMP3 state. Actual native VST
  display notifications reach the offline preview host. These are offline tests.
- Official release.py development installer generated; catalog_check.py with
  --catalog --require-portable passes with zero warnings. ZIP checksums and
  mandatory installer files verified. This checks package format, not public
  availability of the future source repository or real-device behavior.

Logs and byte-specific test receipts are local under build/. The Live III was
unreachable during the first prototypes and is reachable during 0.0.3 work.
Read-only inspection of stock skin component types was performed; no Geonkick
UI test or device CPU measurement has taken place. The user has approved
installing the next 0.0.3 visual build. On that approval the existing Geonkick
folder was updated from 0.0.2 to 0.0.3: all 582 package files were checksum
verified after staging and again after replacement. The previous folder was
backed up locally. MPC.settings stayed byte-identical and the running MPC process
was unchanged. After the user explicitly approved the restart, the MPC application service
was restarted once and returned to active/running with a new process. The
startup journal shows the updated Geonkick VST instance being created and
initialized. Hands-on GUI/audio verification is still pending.

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

## 0.0.4 — finer graph and readable native text

- Native ASan/UBSan/leak suite and actual ARM64 VST/engine checks under QEMU pass.
- 432 waveform columns match captured audio within half of one pixel-height
  quantization step; the envelope uses 85 height levels. All 474 earlier VST
  entries keep their indices/ranges/defaults. Stored project state is unchanged.
- Native skin checks have no touch-overlap or edge warnings. Plot strips are
  9312/16320 pixels tall, within the image limit. Package validation: zero warnings.
- With explicit user approval, 606 package files were installed and hash-verified;
  the prior plugin folder was backed up locally. MPC.settings stayed identical
  during the file update. A separately authorized app restart returned to running.
- Read-only before-capture confirms the 0.0.3 block/font issue. After the 0.0.4
  restart MPC shows its track-type selector; the new plugin screen is not yet
  visually verified. No project/instrument selection or UI input was performed.

The actual installed 0.0.4 shared library also passed the isolated framework VST
host suite on Live III from a temporary low-priority analysis process. This is
a binary/host test, not a graphical MPC-app test or real-time CPU benchmark.
The temporary helper was removed.

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

## 0.0.5 — fine controls and editing correction

- Native ASan/UBSan/leak and ARM64 QEMU actual VST/engine tests pass.
- All nine frequency adapters pass six forward wheel steps, six backward
  Q-Link steps, host automation echoes, 20/12000 Hz endpoints, and old linear
  automation. At 55 Hz the largest step in the six-step test is 0.1614 Hz.
  Explicit adapter echoes and delayed display-thread echoes preserve an old
  6010 Hz automation value instead of snapping it to the new pitch grid.
- Framework tests cover whole-range continuous integer sweeps in both
  directions, reversals and repeated endpoint-clamped wheel/Q-Link events.
- Independently checked shortened envelopes: decay 40/500 ms and pitch time
  40/200 ms produce distinct slopes at length 30 ms. Decay edits extend length;
  envelope context, milliseconds/percent adapters, noise visibility selector
  and FM with a disabled destination are exercised.
- All original 1482 parameter identities, ranges, defaults and option lists
  remain intact. GKMP1/2/3 migration and round trip still pass.
- Final skin check has no findings. Actual-value offline previews reviewed
  for SYNTH, tone OSC and noise OSC; captions follow conditional controls.
- New version is prepared locally only; installation, restart and hardware
  acceptance of 0.0.5 have not been authorized or performed.

The user reported confusing controls, missing/doubled text, coarse frequency
and ineffective decay on 0.0.4. These reports supersede the earlier pending
status for that early build; they do not establish a passing hardware test.

## 0.0.6 — UI callbacks separated from audio

- Reproduced 0.0.5 with the actual native VST and a host that sleeps 20 ms in
  UI callbacks: 37 callbacks ran on the rendering thread, worst block 100.383 ms.
  The 128-frame/44.1 kHz period is 2.902 ms. This is a synthetic stress fixture,
  not a measurement of an MPC underrun's duration.
- With 0.0.6, native ASan/UBSan/leak and ARM64 QEMU suites pass, including the
  slow-UI regression (zero UI callbacks on rendering thread), fine frequency
  echoes, independent DSP oracle, state migration and cache lifetime tests.
  Native slow-UI block timing remains diagnostic;
  zero audio-thread UI callbacks is the regression assertion.
- A separate low-priority analysis process on Live III loaded the candidate
  .so from a temporary directory: zero audio-thread UI callbacks, worst measured
  block 0.045 ms in the 16-block slow-UI fixture. It is not an MPC-app session,
  long-duration stress test, or certification that all audible crackles are fixed.
- Isolated Live III worker diagnostic, Init routing (one audible layer), 4-second
  sound: 0.0.5 distortion cases were about 58–65 ms to become ready; initial candidate
  cases about 50–58 ms before skipping muted layers; final candidate distortion
  cases about 28–36 ms after skipping them. This includes scheduling and polling, is not a controlled
  CPU benchmark, and does not cover nine active oscillators/effects.
- Muted layers are skipped during synthesis. All nine isolated oscillator/effect
  paths and the independent baseline signal oracle still pass.
- View changes do not rerun synthesis; a view switch retains byte-identical
  noise audio. Output/tuning/tracking/playback edits do not rerun synthesis.
  OUTPUT gain smoothing is verified during a sustained hit.
- Final skin check has no findings. The wrapper also compiles without the async
  option/display thread. The dependency patch reverse-check passes.
- Candidate tests wrote temporary analysis files only, removed afterward. No
  candidate installation, settings edit, MPC input, service restart or project
  change occurred. Installed version remains 0.0.5 until new explicit approval.

Read-only sampling of the user's 0.0.5 interaction session showed a busy UI
thread and touch-processing lag reports. These support investigating UI stalls;
CPU samples alone do not prove the cause of every crackle. The next gate is
listening to the same actions in the MPC app with this exact candidate.

## 0.0.7 — high edit-time CPU investigation

The user's 0.0.6 listening test failed: frequency and other edits still crackled
and the user observed CPU above 200%. Build identity was independently verified
against the exact packaged .so hash and the real host's version-6 initialization.
The previous short isolated host test was insufficient for this failure mode.

Read-only 0.0.6 Live III measurements:

- Two-second thread samples over about a minute: roughly 49% of one core for
  the main UI thread, 28% for gk-render and 22% for central audio processing.
  The largest whole-process two-second average was about 155%; this sampling
  does not resolve or disprove the user's shorter 200%+ peaks.
- A separate 25-second CPU-clock/callchain sample at 199 Hz per selected thread
  showed most UI/audio-processing samples inside the MPC executable. The
  synth worker spent time in original envelope/oscillator synthesis and libc;
  the plugin's playback AudioWorker was a small part of sampled CPU work.
  These are CPU samples, not a measure of blocked time or proof of a specific
  private MPC function. Named MPC hotspots were not resolved.
- The kernel does not expose per-thread schedstat here. No process was stopped
  or patched. Temporary profiler was removed. Raw maps, symbol metadata and
  samples are private ignored build artifacts, not release assets.

Concrete source findings and candidate checks:

- 1296 graph parameters plus a derived visibility selector were advertised as
  automatable despite being generated display values. The candidate marks
  these 1297 read-only/non-automatable; original editable automation stays.
- Graph feedback is deferred during input, then sent after a 180 ms quiet
  interval. Native/ARM64 actual-library tests verify zero graph callbacks during
  90 edits, continued ordinary feedback, final refresh and stable echoed pitch.
- Rapid input formerly canceled the in-flight sound on every new revision.
  The candidate coalesces starts to about 30 Hz and completes intermediate
  snapshots. In native/ARM64 tests 240 fast changes caused 7–8 render starts
  over about 0.26–0.27 seconds, with audio publications before the gesture ends.
- Full native ASan/UBSan/leaks and ARM64 QEMU suites pass, including original
  DSP oracle, fine pitch, slow-UI callback, old state and all oscillator paths.
  Final skin check and default synchronous-wrapper compilation pass.

These changes target measured and source-confirmed pressure points. They do not
establish that all MPC-app crackles or CPU peaks are fixed. Required acceptance:
real MPC host must still draw non-automatable graph values after a gesture,
maintain Q-Link sync, and improve CPU/audio during the same editing actions.
0.0.7 is local only; installed version stays 0.0.6 until explicit new approval.

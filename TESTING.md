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

## 0.0.8 — rounded fine input and shared graph invalidation candidate

The user reports 0.0.7 is much better, but redraws still crackle and fine moves
can jump on data wheel, Q-Links and touch. A local reproduction quantizing
small normalized requests to 0.001 caused 22–23 pitch-step jumps at 14 positions
in the 0.0.7 range. The integer-nudge branch treated exact-position requests as
absolute jumps. This reproduces a concrete fragile input case; it is not a
capture proving that the MPC uses precisely this rounding or that every reported
jump shares this cause.

- Native ASan/UBSan/leaks and actual ARM64 VST/engine suites under QEMU pass.
- Rounded small requests across the complete pitch range now move one step;
  millisecond integer-boundary nudges, immediate host echoes, old Hz automation,
  endpoints, continuous sweeps and reversals still pass.
- All 1298 graph/view values are non-automatable. No per-column host feedback is
  sent. A shared graph revision is deferred during edits, then notified; the
  fixture directly queries actual column values on that signal. This models a
  visible UI re-read; it is not a native MPC renderer test.
- All 5184 plot widget instances across the four SYNTH subpages have the shared
  invalidation handle. Final skin check has no findings. The default wrapper
  compilation and dependency reverse-patch check pass.
- Actual-value offline SYNTH/tone/noise previews render the compiled VST state.
  The preview asserts a revision notification and no column notifications.
- Original 1529 VST entries retain their indices and ranges; the one revision
  parameter is appended. The 186-value GKMP3 project format is unchanged.

No reliable native-tab notification was established. Current-tab guessing from
getParameter reads is deliberately avoided. This candidate relies on MPC's own
handling of invalidation and hidden components; redraw/page-return behavior and
redraw-time audio still require hardware acceptance. Nothing new was installed
or restarted. Device screenshots, stock-skin references and any diagnostic data
remain private ignored artifacts.

## 0.0.9 — shared refresh retained, larger slider hit areas, input observation

User hardware report: 0.0.8's graph does not redraw and the sporadic jumps remain.
The offline model therefore did not validate either complete real-host behavior.
The candidate keeps the shared-refresh optimization and adds one post-gesture
host UpdateDisplay from the reporter, instead of restoring column broadcasts.
The actual-library fixture asserts the shared signal/rebind and zero column
notifications. Real repaint and audio results remain pending.

- Native ASan/UBSan/leaks and actual ARM64 QEMU VST/engine tests pass.
- Native slider hit areas are enlarged with transparent padded frames. Every
  original visible pixel in every frame was compared and is unchanged. Main
  envelope hit height is 64 px, other constrained sliders 41/44 px in this skin.
  Native skin overlap/edge checks have no findings; actual touch feel is pending.
- Actual-value offline SYNTH/tone/noise composites render successfully.
- A bounded asynchronous diagnostic records fine-control incoming/accepted
  values on MPC only, under a private /tmp prefix. No producer-side file I/O.
  Logger lifecycle and bounds were exercised under ASan using an analysis
  executable named MPC; its own temporary test files were removed.
- Original parameter indices, 186 stored controls and independent DSP oracle
  remain unchanged. No new claim that the remaining input jumps are fixed.

The user has now granted standing permission to install our new plugin builds
and restart the MPC app until revoked. All other device access remains read-only
except our temporary /tmp diagnostics. Deployment still backs up, stages and
hash-verifies this plugin, without editing global settings or projects.

## 0.0.10 — actual accumulated input stream

The user confirms 0.0.9 redraws again and the tested redraw actions no longer
crackle on Live III. Its graph path and slider geometry are retained.
Private incoming/accepted traces exposed the remaining jump mechanism: raw
MPC events accumulate by 1/128 independently of the accepted fine value. The
old heuristic compared their growing separation to a 10%-range cutoff, then
accepted an unintended absolute position, causing about 229–249 pitch-step
jumps. The previous rounded-input test did not represent this stream.

A generated regression reproducing the stream (not publishing raw user logs)
fails against 0.0.9. The candidate follows individual raw deltas instead:

- All nine oscillators: 90 accumulating forward/backward events at both wheel
  and Q-Link rates, including a clamped tail, remain one fine step per event.
- Existing immediate feedback-echo, quantized-input whole-range, continuous
  sweep/reversal, endpoint and original physical-Hz automation tests pass.
- Native ASan/UBSan/leaks and actual ARM64 VST/engine tests under QEMU pass.
- The fine-control/echo regression was repeated four times after correcting
  control-history serialization and synchronous self-echo handling. All pass.
- Shared graph/rebind fixture still has zero column callbacks; skin overlap
  checks, default legacy wrapper compilation and patch reverse-check pass.
- No audio-engine or graph/touch geometry changes are introduced in this build.

Fine-control device acceptance remains required. The temporary diagnostic is
retained for that check; raw captures stay private and are removed from /tmp
after analysis. The user's standing install/restart authorization remains active.

## 0.0.11 — direct touch, fine Q-Links

The user confirms 0.0.10 real value jumps are resolved, but observes a brief
visual knob rebound and wants coarse/direct touchscreen motion with fine
Q-Links. Previously one native Knob handle represented raw touch input and the
accepted fine value. The candidate keeps the fine Data/Q-Link binding and moves
native Knob motion/display to an independent continuous Touch handle sharing
the same physical engine value.

- Native ASan/UBSan/leaks and actual ARM64 VST/engine suites under QEMU pass.
- All 46 continuous adapters follow direct requested normalized positions,
  preserve intermediate pitch fractions, refresh their readback, hand over to
  a fine Q-Link step and preserve old 6010 Hz values through feedback echoes.
- Accumulated raw fine-stream/reversal tests, shared graph/rebind fixture,
  independent DSP oracle, presets/state and cache/voice lifetime still pass.
- Structural split verified on 265 widget instances. Existing Q-Link map is
  byte-identical to 0.0.10; old 1530 parameter identities/ranges/defaults remain.
  All state still stores 186 physical controls, not duplicate touch values.
- Final skin check and actual-value offline rendering are required for package.

The native MPC must still verify that child Touch handles receive dragging,
parent Data keeps Q-Link/wheel focus fine, and the cosmetic rebound is gone.
Point-envelope editing was investigated in the original source and documented;
no editor/state-format change is included in this build.

## 0.0.12 — fixed-point Amp/Pitch editor, local build only

The user confirmed the 0.0.11 Touch/Q-Link changes work. For 0.0.12 the user
explicitly requested no installation or MPC restart. No device writes or device
interaction are part of this build's validation.

- Native ASan/UBSan/leak and actual ARM64 shared-library / engine suites under
  QEMU pass. The independent original Init audio oracle still passes.
- Converting a nontrivial simple Amp and Pitch envelope to five linear points
  preserves the full captured audio exactly in the fixture (maximum error 0).
- All 18 oscillator/envelope contexts, positive segment times, dynamic fine /
  continuous point controls, first-point fixed time, custom silent amplitude,
  alternating extreme pitch/time values and finite audible synthesis pass.
- Custom curves and fractional values round-trip in GKMP4; 186-value GKMP3
  chunks load with Simple curves. Truncated/non-finite states are rejected
  without partial mutation. Switching mode preserves custom edits; Copy Simple
  replaces them; editing legacy attack switches the Amp curve back to Simple.
- Actual VST fixture covers direct fractional point touch, twelve accumulated
  Q-Link events with asynchronous feedback, and changing selected point.
- All previous 1576 VST parameter definitions remain identical. The new total
  is 1782, with 385 stored values. Structural skin checks cover 18 native
  Envelope instances, their 162 point/time bindings, conditional modes, and
  separate fine Data / continuous Touch point knobs.
- The existing shared graph refresh and zero audio-thread UI callback tests
  remain passing. Temporary input tracing is disabled in this build.

Native Envelope geometry and gesture interpretation are not verified by QEMU
or an offline image. Those require a later MPC test. Offline point-editor
previews approximate the native widget with actual compiled VST values; they
are layout checks, not device screenshots or proof of hardware performance.

Final skin was regenerated from the scripts and matched the reviewed TUI
byte-for-byte; final native skin checks report no findings. Official portable
OpenMPC catalog/package validation reports zero warnings. Installer file hashes,
mandatory scripts, exact tested-library receipts and a scan for private local
paths/device details/input logger strings pass. The installer remains under
build/releases only; no Desktop copy, installation or MPC restart was performed.

## 0.0.13 — simultaneous waveform and touch-only point editing

User confirmed XY-like point dragging in 0.0.12 and requested seeing the waveform
while editing. The 0.0.13 change is in native skin generation and version metadata;
DSP and saved parameters remain unchanged.

- Native ASan/UBSan/leaks and ARM64 QEMU actual-VST/engine suites pass, including
  original audio oracle, custom envelope/state compatibility, quiet-time shared
  refresh and zero audio-thread UI callbacks.
- Actual-VST-value previews reviewed for Simple, Amp and Pitch. The mixed-hit
  waveform stays visible behind the point curve. Its readonly components ignore
  touches and are ordered below the native Envelope.
- Structural checks verify all 1782 definitions unchanged, 18 native envelope
  bindings, 864 waveform widgets reusing existing viz_p/viz_n and graph_revision,
  no Q-Link action/hardware focus for point controls, and only Length mapped on
  ENVELOPE. All Q-Link mappings outside that page match 0.0.12 exactly.
- Final skin check has no findings. Portable OpenMPC installer validation passes
  with zero warnings. No new synthesis pass, derived parameter, graph callback
  burst or periodic polling path is added by the skin.

Offline image geometry is an approximation. Native waveform/curve alignment and
redraw performance require user acceptance on the MPC; the additional visible
waveform widgets are not covered by an offline CPU/audio claim.


## 0.0.14 — six menu groups, rotary Q-Links and click boundaries

Native ASan/UBSan/leak and actual ARM64 shared-object/QEMU suites pass. New
signal-boundary regression also runs successfully in a private isolated engine
helper on the real Live III: 30 ms kick final-boundary max adjacent-sample delta
0.0018601; Cut retrigger's first sample matches continuous outgoing audio,
max transition delta 0.0124840 in the defined low-frequency test; after the short
fade it matches the immediately started new hit within 1e-6. Rapid retriggers
remain finite and CC120/123 clear all queued tails. Temporary helper removed.
This is an isolated signal test, not MPC-app listening acceptance or a CPU bench.

Five-ms smoothstep endings replace the old 64-sample linear fade. A fixed 221-sample
additive tail ring continues cut/stolen voices with a smooth fade while releasing
their cache references. New hits start immediately. Existing pending tails are
preserved during retriggers; panic/project restore clears the ring. No runtime
allocation, lock, synthesis wait or UI callback is added to audio processing.
Default chirp oracle max error 3.87430191e-7; existing Simple→Points conversion
remains exact in its test. Independent cache/graph, fine/direct touch, MIDI CC20,
NRPN, envelope/state/GKMP migration, per-oscillator FX and blocked-UI tests pass.

All 25 pages are reachable on six main keys. Layer pages alternate OSC/FX pairs;
Envelope moves onto key5; MASTER groups Mix/FX/Presets on key6. The first layout
QLINK seed stays unchanged for CC compatibility. Final RowTopDown screen maps
contain only actual rotary widgets; no point controls, switches, selectors or
sliders. Per-control touch-only clones preserve native Button/Touch data handles.
Native frame checks enclose full names/values; existing 18 native envelopes/162
point handles and 864 mixed-waveform columns remain wired, quiet-time invalidation
unchanged. Previews reviewed, including corrected banner/caption and mixer spacing.
Geometry and fine/direct point wiring checks pass. Hardware UI acceptance pending.

Packager uses current reviewed templates at 42420b6717b93e954ef3992d9a7aa0909f1e4bec; explicit
plugin.json format:vst2 checked. Current catalog/portable/checksum/privacy checks
pass, zero warnings. Installer SHA-256:`839b1a837cd422fe89bf37bc0df6589c6e395f6c78e548c476fa707b45a1a6cd`. No public fork/release or formal
Store benchmark; no Desktop copy. Device deployment/acceptance recorded below
once performed. See TESTPLAN-0.0.14-DE.md and QLINKS-0.0.14-DE.md.


0.0.14 deployment under standing authorization completed: previous 0.0.13
backed up locally, 1502 plugin files staged and hash-verified, settings unchanged
during file replacement. MPC application restarted; active/running confirmed.
Installed stripped SHA-256 matches the checked installer:
`5bb0f649c0330c32aac8cd32d3bcc5c9277b3b6927d558b379177716f367a619`.
Installed plugin.json explicitly reports format:vst2. No global settings/project/
unrelated-plugin changes, full device reboot, Desktop copy or public release.
Actual menu/touch/Q-Link and short-kick/Cut listening acceptance remains pending.


## 0.0.15 — visual overhaul, no synthesis changes

Native ASan/UBSan/leak and actual ARM64 shared-object/QEMU suites pass, including
all 0.0.14 signal-boundary/Cut tests. The DSP sources and parameter table are
unchanged. One-off comparison against the checked 0.0.14 skin verifies all 6895
native control/display placements, action handlers, knob/button gestures and
non-colour envelope fields unchanged; Q-Links.json is byte-identical. Existing
25-page/six-key rotary-only, point-envelope and full native bounds checks pass.
Only artwork, text/envelope colours and native value backgroundData change.
No extra input-catching components or display callback paths are introduced.

Offline native-value previews reviewed for SOUND, tone/noise OSC, MASTER/MIX,
global effects and Simple/Amp/Pitch editors. Header/selector labels remain clear,
conditional numeric backgrounds follow native widget visibility, and actual
waveform/point geometry remains unaltered. The original new browser tile is
270x110 and shares the geometric percussion mark. These are offline visual
checks, not MPC screen-capture or listening acceptance.

Current pinned installer/catalog checks report zero warnings; explicit VST2
metadata, archive checksums and privacy scan pass. Installer SHA-256:
`1f2753b8735c5433c4e3eac58008bc33f8e7ed8fa6150de7d361c825e563aca7`.
Previous 0.0.14 backed up locally; 1488 package files staged/hash-verified,
settings unchanged during replacement. MPC application restarted active/running,
installed stripped SHA verified:
`8820c78c7517355e9a50a2c87bcd20132d9bcefa2552018a395ddcf2e7fde990`.
No global settings/project/unrelated-plugin changes, full reboot, Desktop copy,
public release or formal benchmark. Real-device visual/touch acceptance pending.


## 0.0.16 — canonical feedback and centred preset buttons (2026-11-20)

Reproduced on actual VST code: echoing a getter's exact normalized o1_freq value
through the old setter rounded its physical value via %g and selected oscillator 1
again. The small opt-in CANONICAL_CONTINUOUS_ECHO patch returns early for exact
ordinary continuous echoes. It does not change relative fine-step handling,
switches, momentary actions, popup/step handles or genuinely changed input.
The documented clean-base wrapper patch sequence applies successfully.

New regression covers 9 targets x Amp/Pitch, with/without audio rendering, manual
whole-control echoes and async graph/rebind feedback. Fractional physical values
remain identical; a real o3_level edit still selects oscillator 3. Native sanitizer
and actual ARM64 shared-object/QEMU full suites pass, including all existing
fine/direct/raw-delta, point/state, waveform and short-kick/Cut tests. Running
the new regression against the frozen 0.0.15 ARM64 library fails as expected
(focus jumps to 8 during an exhaustive control echo), confirming the regression
catches the old behavior. This is not claimed as a hardware touch acceptance test.

Moved the preset grid 105 px upward. All 8 native buttons now fit inside the painted
panel, with equal left/right margins and top/bottom margins differing by 1 px.
Offline preview reviewed; input bindings and preset identities are unchanged.
Current OpenMPC contribution/review guidance was reread. Updated packaging tools
pin to 1af05b911e8580c8c2e473a1cc10119132e06eca, incorporating drive-folder detection, safe case-only
retirement handling and BOM/CRLF checks. Current catalog/portable/text/checksum/
privacy validation passes with zero warnings. No formal CPU benchmark was run.

Final installer SHA-256:`20d2078778e735dd7e56a714d115dc2c9366aff0fc931ec18641618f000e374f`.
The Desktop copy Geonkick-MPC-Gen2-0.0.16-dev1.zip is byte-identical to it.
After initially asking to leave the MPC alone, the user gave a fresh specific Go
for this build. Previous 0.0.15 backed up locally; 1488 files staged/hash-verified;
settings unchanged during replacement. MPC app restarted active/running and
installed stripped SHA verified:`bb720a443f8c593473532a0b4834088b9d8e5a8d61aa970cfe40b3b22ae097b0`.
No global settings/project/unrelated-plugin changes, full reboot or public
release. Actual native selector/preset acceptance remains pending. Future device
access still requires fresh authorization; the prior automatic grant was revoked.

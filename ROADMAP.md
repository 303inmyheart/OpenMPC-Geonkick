# Geonkick MPC roadmap

Target: Gen2 / aarch64, MPC OS 3.10.1+. Early builds tested by the user on Live III; new builds require separate validation.

## Current plan

1. **Stable base and device controls** — working through0.0.19, confirmed by
   the user on Live III. Continue checking each new build separately.
2. **Point envelopes** —2–16 points with cubic curvature, waveform overlay,
   absolute XY editing and numbered selection are implemented. Free Bezier
   control handles / arbitrary tap-to-insert remain deferred extensions.
3. **Sound-engine parity** — WIDE/CLASSIC filters completed and accepted0.0.19.
   Humanizer removed in0.0.21 at the user's request; use the MPC's own features.
   Additional filter/resonance and other modulation-envelope UI work is skipped
   at the user's request. See [engine audit](docs/ENGINE-PARITY.md).
4. **Pad foundation before import** —0.0.21 implements16 independent sounds,
   editing selection, consecutive MIDI-note mapping, per-pad Poly/Cut/Gate,
   shared worker/caches and complete kit/project persistence. User confirmed the
   learned hardware mapping in0.0.22 works. Stereo output;16 shared voices.
5. **Native MPC starter presets** —0.0.28 implemented eight MPC starting
   recipes plus a16-pad kit and removed MASTER/PRESETS. These were not the
   original factory library. Native browser hardware acceptance is still separate.
6. **Original Geonkick factory presets — NOW FIRST** — user explicitly moved
   this before redesign. Convert102 original.gkick files and7.gkit files from
   all9 pinned factory packs to native MPC.xpl, retaining original DSP envelopes,
   FM/seed/pitch/sample settings and pack-specific attribution/licences. Preserve
   immutable source identities in saved kits. Arbitrary user.gkick/.gkit file
   import remains a separate future browser feature.
7. **Redesign — after factory presets** — revisit spacing/hierarchy/navigation.
   Include MASTER/SAMPLES selected-WAV preview/stop before Load, without replacing
   the assigned sound. Existing PLAY PAD auditions the assigned sound only.
   Later kit work: copying, mute/solo/choke groups, audio export and routing audit.
8. **Public release** — small KIT/pad-learning tutorial, hardware regression,
   exact release-binary Store benchmark, source/licensing/reproducible-build
   review, release and catalog PR.

Before release: add a small in-plugin tutorial on MASTER/KIT for pad learning,
including stopping playback, selecting a bank, pressing1–16 in order and learning
again after note-layout changes. Sound-tab envelope following was moved forward
into0.0.23 at the user’s request.

Historical build notes follow; the latest entries override older pending items.
A formal Store benchmark remains reserved for the finished release binary.

The remaining JC303 feature-parity items stay deferred. Following targets:
Fire, Wolf Shaper, BYOD navigation feasibility review, then Surge.


0.0.12 was subsequently installed on explicit request. User confirmed that the
points can be moved in two dimensions. 0.0.13 keeps the rendered mixed-hit waveform
visible behind the draggable envelope, removes point-editing Q-Link assignments
and hardware focus, and retains the existing quiet-time graph refresh. Five fixed
points remain; arbitrary insertion/deletion is a separate future task. Combined
view alignment and redraw performance need device acceptance.


## Current 0.0.14 — menu/Q-Link redesign and signal boundaries

- Six main keys: SOUND; LAYER1/2/3; ENVELOPE; MASTER. All 25 existing pages are
  reachable; oscillator/FX pairs alternate within a layer. MASTER groups MIX,
  global effects and presets. MIX now exposes all layer gains and Output,
  plus Tune/Key Track/Playback/Audition; the waveform remains visible.
- Q-Links only for rotary knobs, RowTopDown groups with full native caption/value
  bounds. Switches/selectors/sliders and envelope-point controls are touch-only.
- Conditional tone/noise/FM knobs keep stable positions; noise density now has
  its own lower-row position instead of sharing the frequency knob's location.
- Smooth 5 ms cached sound endings and bounded continuous 5 ms Cut/voice-steal tails;
  new attacks start immediately, MIDI panic clears tails. No synthesis/UI work
  moves onto the audio thread. Signal regressions pass natively, under ARM64/QEMU
  and in a private isolated Live III engine test. Listening acceptance pending.
- Current reviewed OpenMPC packaging pin, explicit plugin.json format:vst2 and
  current installer templates; catalog/package checker zero warnings.

Next: actual MPC menu/touch/Q-Link and short-hit/Cut listening validation. Then
resume arbitrary point insertion/deletion/Bezier feasibility and the previously
listed sound/kit tasks. Keep the existing quiet-time redraw, coarse direct touch,
fine hardware input and waveform/point overlay; do not regress those to old paths.
A formal store benchmark remains reserved for a final release binary.


## 0.0.15 — Percussion Lab design

Graphite panel hierarchy, original rotary strips and selector/switch artwork,
recessed live values, green waveform/amber envelopes, layer/page indicators and
matching original Sounds tile. Waveform/editor data, native gestures/targets,
Q-Link maps, fine/direct touch and 0.0.14 audio paths are preserved. Numeric
recesses use native backgroundData rather than added input-catching Images.
Native/ARM64/point/geometry/installer checks pass; installed under standing
permission. Live III visual/touch acceptance remains pending. No formal benchmark
or public release. Next work remains the previously deferred envelope/kit/parity
items and actual MPC listening/interaction validation.


## 0.0.16 — focus feedback and preset alignment

Exact continuous host echoes are ignored before decimal conversion, preventing
oscillator-focus changes and physical-value rounding during skin rebinds. All
nine targets and Amp/Pitch are regression-tested while stopped/rendering; genuine
knob edits still select their oscillator. The nine fine adapters/accumulated raw
input tests remain passing. Eight preset buttons are fully inside and centred
in their painted panel; horizontal/vertical margins checked from actual native
bounds. Current packaging tools reviewed and updated, zero checker warnings.

Installer delivered to Desktop and installed under a fresh build-specific Go;
MPC application restarted. Latest on-device selector/preset acceptance pending.
No formal Store benchmark, public Geonkick release or renewed blanket permission
for future device access. The JC303 Plugin Manager image issue is recorded in
JC303's roadmap separately.


## 0.0.17 — variable point envelopes (local candidate)

- Amp/Pitch on all nine oscillators: 2–16 points, interior insertion/deletion,
  cubic Bezier segment curvature, touch-only selection/value/time/curve controls.
  Adding a point subdivides the existing cubic without changing its shape.
- Full actual-DSP curve over the mixed-hit waveform; native endpoints plus
  three banked inner grips. Previous/next selection is available. Hardware
  acceptance is required for group changes, sparse 2/3/4-point placeholders,
  fast gestures and quiet-time redraw. Installed after the user renewed standing
  update permission; MPC application restarted and exact binary hash verified.
- GKMP5 and larger opt-in VST chunks, exact GKMP1–4 migration, fixed original
  parameter identities; bounded worker-baked curves and immutable key lookup.
- Remaining desktop envelope parity: free two-dimensional Bezier control handles,
  arbitrary tap-to-insert coordinates (ADD currently splits the selected segment),
  more than 16 points if practical, and original desktop preset interchange.
  Current native fixed-five editor has no documented raw canvas touch callback.

After acceptance, continue useful filter range / sound parity, sample loading,
external presets/export and eventually sixteen-instrument kit architecture.
Formal benchmark only on the final release binary. No public release claimed.


0.0.17 delivery: native ASan/UBSan/leaks, actual ARM64/QEMU shared-library,
point/state/signal/focus tests and native skin geometry pass. Current pinned
OpenMPC checker reports zero warnings. 1500 plugin files staged/hash-verified,
prior0.0.16 backed up locally; settings unchanged during replacement. Necessary
MPC application restart completed, service active/running and installed binary
matches the package. No new on-device touch/hearing acceptance or formal CPU
benchmark claimed. Next: the focused0.0.17 touch/curve test, fixes if needed,
then useful filter range / remaining sound editing parity.


## 0.0.18 — hardware-reported point alignment and sticky actions

Live III screenshots exposed incompatible time spacing in the five-stage Pitch
widget and latched point-action highlights while stopped. Replace it with native
absolute XY editing, all16 readonly markers and numbered selection of existing
points. The amber selected point uses the same time/level rectangle as the curve
and waveform; no grip banks or sparse placeholder points remain. Touching empty
space moves the selected point; it does not select the nearest point or insert.

Idle action feedback resets highlights without audio callbacks, including a
second debounced high from finger release. Selection/status refresh without
synthesis. GKMP5, original2935 VST indices, curves and quiet-time audio/plot
behavior remain compatible;35 readonly/absolute UI channels are appended.

Next gate: user checks numbered selection, XY alignment after settling, all four
point buttons while stopped/playing and two-point endpoints. Then continue
filter range / remaining sound features, followed by samples/presets and kits.
Desktop free Bezier control handles/tap-to-insert remain deferred. No Store
benchmark before the final release binary.


User reports manually installing the latest build and that it works. The exact
installed version could not be independently confirmed because SSH was
unreachable at the final check. Do not claim an automated0.0.18 deployment or
restart; none was performed. Keep the successful user report distinct from the
specific new XY/button hardware matrix until the installed version is known.
0.0.18 local package passes native sanitizers, actualARM64/QEMU, geometry and
current pinned installer checks with0warnings. The stopped-button regression
fails on the frozen0.0.17 shared object and passes on0.0.18.

Recommended next functional task: phase3 sound-engine parity, beginning with a
stable useful filter range/resonance expansion. Check gain, parameter transitions
and old-project behavior. Then remaining original-engine differences; sample
loading and external presets follow in phase4, kits in phase5.


##0.0.19 — phase3 filter range/resonance

- WIDE20 Hz–20 kHz, Q0.5–20, stable trapezoidal LP/HP/BP on the global filter
  and nine oscillator filters. Cached coefficients, double state precision;
  band-pass centre gain normalized. CLASSIC retains old arithmetic/ranges.
- Independent touch-only model selector on each FX page; cutoff5-cent hardware
  steps / continuous logarithmic Touch; resonance0.1-percent steps / direct Touch.
  Existing Q-Link row/frame structure and historical first CC seed are retained.
- Old GKMP1–5 states load CLASSIC automatically; new Init uses WIDE. Factory
  hats keep CLASSIC. GKMP6/1551 values; original2970 VST IDs unchanged, total3040.
- Native sanitizer and actual ARM64/QEMU tests include162 independent-response
  fixtures and host controls. Actual0.0.18 vs current libraries render30 legacy
  project fixtures with zero sample difference. Existing held hits remain exact
  while filter edits render asynchronously. No formal Store benchmark.

Next: actual MPC cutoff/resonance/mode and hearing acceptance, then remaining
engine-parity review (including which original sound-editing functions are still
useful). Phase4 sample loading and original preset interchange follow; full kit
architecture stays phase5. Existing free Bezier handle/tap-to-insert limitations
are unchanged.0.0.18 was confirmed installed via read-only version check before
this update; the user reported it working. No touch acceptance for0.0.19 claimed yet.


0.0.19 deployment completed under standing permission:1626 plugin files staged
and hash-verified, previous0.0.18 backed up locally, settings unchanged during
replacement. Necessary MPC application restart completed; active/running and
installed library hash matches the tested package. User's0.0.19 hearing/touch
acceptance remains pending. Phase3 filter part is implemented; remaining engine
parity review and phase4 sample/preset work are the next roadmap items.


##0.0.20 — original Humanizer

User confirmed0.0.19 working. Add MASTER/HUMANIZER between FX and PRESETS:
Off/On (touch only), velocity0..30% and timing0..30ms, two rotary-only Q-Links.
Use the original GPL humanizer/randomizer; add fixed bounded sample scheduling
for this port's cached-hit playback. Default/presets/older states are OFF.

Pending hits pin their arrival sound, velocity, tuning and playback mode. Cut
acts at the delayed onset. Gate note/channel matching, Panic, reset/state and
bounded overflow cancel pending references safely. No allocation, sleep or
synthesis wait occurs on the audio thread. Humanizer edits skip resynthesis.
GKMP7 stores1554 values; GKMP6 still consumes1551. Old3040 VST identities frozen,
total3047; six main keys retained, MASTER has four subpages (26 pages total).

Next: hardware timing/velocity and saved-state acceptance; then remaining
original sound-envelope controls, followed by sample/preset work. No Store
benchmark or public release of this development build.


0.0.20 deployment completed:1640 plugin files staged/hash-verified, previous
0.0.19 backed up locally; settings unchanged during replacement. Necessary MPC
application restart completed active/running; installed binary hash matches the
package. Native sanitizers/full ARM64 and current package checker pass (0warnings).
Actual0.0.19/current libraries render30 GKMP6 old-project fixtures with maximum
sample difference0. New Humanizer hardware/hearing acceptance remains pending.


## 0.0.21 — sixteen-pad foundation; Humanizer removed

- MASTER/KIT selects one of16 independent instrument states. Each sound retains
  the complete existing engine and its eighteen Amp/Pitch envelopes. Per-page
  selected-pad readout; sound presets/Init affect only the selected sound.
- Single (default/legacy chromatic behavior) and16 Pads modes; configurable first
  MIDI note0..112, default36. Kit mode assigns16 consecutive notes and ignores
  others. Selection is touch-only; routing switches have no Q-Link assignments.
- One DSP worker across all pads with the existing global30-start/s budget,
  round-robin fairness and immutable per-pad caches. Initial identical sounds
  share one cache/render. Sixteen playback voices, independent pad CUT/GATE,
  continuous per-pad/kit output and retained old-note buffers.
- GKMP8 persists every sound and kit metadata; default rows compacted.1MiB VST
  chunk budget, complete >64KiB kits tested through actual VST close/reopen.
  GKMP1–7 migrate to pad1/Single; Humanizer values are consumed then discarded.
- Seven old Humanizer VST numbers/ranges remain inert, hidden/nonautomatable;
  no Humanizer UI, scheduler or saved fields. Extra envelope work stays skipped.
- Immediate fine-control handover resets raw input history when the selected
  instrument/preset changes; regression fails without the opt-in wrapper patch.
- SOUND pages lose the redundant nine-way upper selector at the user's request;
  the curve follows the last edited oscillator, shown in a readout. The actual
  ENVELOPE page keeps its target selector.

Native ASan/UBSan/leak and actualARM64/QEMU tests pass; these are offline
regressions, not a Store benchmark or device acceptance. Next: Live III test,
then samples, original presets and full bank interchange. No Desktop copy or
public fork/release requested for this build.

0.0.21 final offline validation:all16×18×16-point state occupies238482bytes and
survives actual VST close/reopen. Archived0.0.19/current actualARM64 libraries,
30 GKMP6 fixtures, maximum audio difference0. Installer validation:0warnings.

0.0.21 installed on Live III: plugin files and binary checksum verified, previous
version backed up, settings unchanged during replacement, MPC application
restarted active/running. New pad/kit hearing and Touch acceptance is pending.


## 0.0.22 — learn the hardware pad-note map

User confirmed0.0.21 generally works but hardware Bank A pad8 played sound1 and
sound2 was unreachable. The chromatic36..51 assumption was insufficient for the
track’s Pad Perform note layout. The visual physical-pad grid stays unchanged.

- Learn16 captures each physical pad’s actual note in order. Native status shows
  the next target; duplicate note-ons cannot steal a slot. Abort/partial capture
  retain the previous map; only16 unique notes commit. Touch-only map/learn UI,
  no Q-Links added except the existing rotary controls. Preview bypasses capture.
- Chromatic/Learned routing with a saved16-note lookup; CHROMATIC START affects
  Chromatic only. Normal MIDI after learning does not auto-select or cause UI
  status updates. A changed host scale/octave/map requires matching/relearning.
- GKMP9 adds a validated unique MAP1 table. GKMP8 kits preserve chromatic routing;
  earlier migrations and all1551 sound fields per pad remain unchanged.3056 VST
  entries; first3053 exact identities/ranges retained. No audio synthesis change.
- During learning, status uses explicit shared display invalidation, including
  the first note when selected pad01 has not changed. No polled/audio-thread UI
  text calls, file IO, allocation or new synthesis are introduced.

Next: hardware learn/map acceptance, then samples per pad, original presets and
banks. Future original-bank import should preserve the learned hardware map by
default unless the user explicitly chooses to import/replace note routing.

0.0.22 installed on Live III:1721 plugin files and installed binary verified,
previous0.0.21 backed up locally, settings unchanged during replacement. MPC
application restarted active/running. Actual hardware-learn acceptance pending.
Installer/catalog validation:0 warnings; privacy scan passed.


## Current0.0.23 task — samples and Sound-tab envelopes

User accepted0.0.22 hardware mapping. Continue sample import per pad/oscillator.
Include Sound-tab-specific envelope targets/display now (user moved this forward
from the pre-release list); do not implement the previously skipped extra
filter/resonance modulation envelopes. Preserve quiet-time redraw/audio caches.

Before the release build: add a short on-screen tutorial explaining hardware
pad learning/routing. This tutorial is requested but remains deferred.

User clarification during0.0.23: upcoming presets/banks MUST use native MPC
preset management/Sounds browser. Keep the sample-file browser scoped to audio
files; do not replace the native MPC preset workflow with a new preset browser.


## 0.0.23 — WAV sample import and Sound-tab curve following

- MASTER/SAMPLES native folder/file list,8 visible rows, paging/up/drives,
  per-pad/oscillator import. Filesystem/decoding stay on the existing worker.
  No competing preset browser: future preset/bank work MUST remain MPC-native.
- WAV PCM8/16/24/32, float32/64 and extensible; first channel,44.1kHz conversion,
  first4seconds;10MiB decoded PCM per instance. Failed/invalid/stale imports leave
  existing sound data intact. Import replaces the chosen oscillator’s source and
  initializes flat Amp / original-speed pitch, retaining other oscillators/FX.
- Synth/Sample source selectors and sample labels on layer pages. Sample pitch
  keeps existing fine/direct controls, now in semitones; matching dynamic units.
  Constant pitch avoids repeated exponential evaluation. One-shot source endings
  fade smoothly even if the total cached sound is longer than the source file.
- Immutable asset ownership / deferred reclamation, generation-stamped requests,
  consistent snapshots, held-note cache lifetime. Samples are embedded in VIEW1 /
  SMP1 state extensions (16MiB VST capacity), not external absolute paths.
- SOUND LAYER1/2/3 tabs each bind their remembered oscillator’s curve and sliders.
  All three curves are baked with the completed cache; tab switches need no new
  synthesis. Shared quiet-time invalidation remains. Tutorial still deferred to
  before the release build, as requested; extra modulation-envelope work stays skipped.

Next: hardware sample/browser/curve acceptance, then original presets/banks via
native MPC preset management and Sounds. Before release: small on-screen KIT
learning tutorial, remaining quality review and exact final-binary benchmark.

0.0.23 deployed on Live III:1856 plugin files and stripped binary checksum
verified, prior0.0.22 backed up locally, settings unchanged during replacement.
MPC application restarted active/running. Sample/browser/Sound-tab hardware
acceptance remains pending. Installer validation:0warnings; privacy scan passed.

0.0.24: SOUND layer waveforms now separated as well as envelopes (the0.0.23
waveform still showed the full mix). Worker-only peak taps, cached on tab changes.
Persistent default audio folder via SET DEFAULT / OPEN DEFAULT; plugin-local
preference, excluded from project/preset state. Native MPC presets/banks remain
next; short KIT learning tutorial before release.

0.0.24 was installed with1858 package files verified; the previous plugin was
backed up, settings were unchanged during replacement, and the MPC application
returned active/running with the packaged binary. Native sanitizer and actual
ARM64/QEMU suites and the installer checker pass; fresh on-device waveform and
default-folder acceptance is pending. No final-release Store benchmark.


0.0.25 corrective build: MPC native skin registration creates only Parameter0
through4095 (confirmed by read-only local firmware analysis; no system patch).
0.0.24's new wave/default handles were above that boundary; user reports missing
SOUND waveform, nonworking Set Default and a UI hang on a second instance.
The app was restarted; no fix was installed at that point. Reuse a shared
432-column cached layer view, selected by visible native text formatting; move
Set Default inside the native ceiling. Preserve all physical controls/project
chunks; relock only derived, nonautomatable display slots before public release.
Actual on-device tab following remains acceptance testing, not proven by QEMU.
Native MPC presets/banks next; remove MASTER/PRESETS when native sounds replace
it. KIT tutorial before final release. No benchmark before final release binary.

0.0.25 deployment complete:1859 package files verified, previous0.0.24 backed
up locally, settings unchanged during replacement, MPC app restarted active/
running and installed library matches installer. Native sanitizer/full ARM64
suites and final two-instance display-context regressions pass. On-device SOUND
view following, folder button and UI responsiveness remain user acceptance.


0.0.26: user confirmed25 restored SOUND waveform visibility, but all three views
still showed the same data. A bounded private device trace captured first-tab
then visible-tab caption formatting during native rebuilds. Publishing every
request immediately produced a rebuild feedback loop. Pending contexts now
coalesce for100ms and are adopted only on the normal display scan, with one
revision per stable change. No resynthesis or reduced graph resolution.
Native sanitizer and ARM64/QEMU replay tests pass; archived25 fails the same
feedback-count assertion. Hardware validation is still pending. Next remains
native MPC presets/banks, replacing MASTER/PRESETS; KIT tutorial before release.

0.0.26 installed:1859 package files verified, previous plugin backed up locally,
settings unchanged during replacement, MPC app restarted active/running and
installed library matches the uninstrumented installer. Temporary device trace
removed. Native waveform/tab acceptance remains pending user retest.


0.0.27: user reported26 loses the SOUND waveform after navigating and ENVELOPE
can recover it. Device journal loads26 correctly, with no matching texture/
plugin exception; caption-format contexts remain unreliable across cached pages.
User explicitly approved ONE SOUND page with three layer Touch buttons. The
new selector derives directly from the stored plot_osc target; choosing a layer
restores its remembered oscillator. No inferred tab context/timer or caption side
effects. Shared waveform data directly follows this same selected layer, and the
existing global envelope matches the selected oscillator. MIX/editor keep mixed
audio. The detail pages remain. Six overview Q-Link proxies follow current layer;
coarse Touch to fine handover resets stale raw history. Canonical physical
parameter notifications preserve stable automation targets; recording acceptance
is pending. All physical/state identities remain; only7 unused display slots are
reallocated below4096.25 pages on6 keys; full432-column view. Next after hardware
acceptance: native MPC presets/Sounds and soundbank import, removing MASTER/
PRESETS when native Init/factory sounds replace it; KIT tutorial before release.

0.0.27 installed:1865 package files verified, previous plugin backed up locally,
MPC application restarted active/running and installed library matches installer.
User reports the new overview appears to work and is continuing brief hardware
tests; full Q-Link/navigation/automation acceptance is not yet complete.


## 0.0.28 — native MPC presets and starter kit

Implemented eight MPC starter presets including a complete Init snapshot plus
a16-pad starter kit, using standard full-instance native.xpl files. These are
MPC recipes, not the original Geonkick factory library. Native Save/Load retains
the complete kit, learned mapping and embedded samples. MASTER/PRESETS removed;
INIT PAD on KIT resets only the selected sound. Old parameter/program identities,
GKMP9 format and the accepted explicit SOUND-layer behavior are retained.

Native sanitizer and actual ARM64/QEMU tests pass: authentic state replacement,
all16 independent sound/audio references, learned-map complete VST close/reopen.
All nine portable XPL files/UIDs/version/payload hashes and final24-page skin
validated. Installer checker reports0 warnings. Hardware acceptance pending;
see TESTPLAN-0.0.28-DE.md. No public release or final Store benchmark.

Next: redesign immediately, including MASTER/SAMPLES WAV preview/stop before
Load, without replacing the assigned sound. Faithful original.gkick/.gkit import
remains a later engine/compatibility step. KIT tutorial remains before release.


## 0.0.29 — original factory presets before redesign

All102 original.gkick presets and7.gkit kits from nine pinned packs converted
to native MPC.xpl files.109 original entries plus the9 earlier MPC starter
states give118 browser files. Pack names distinguish them; the adapted AVL kit
is called MPC AVL-derived Kit. Original authors/licences and change notices ship
in Factory-Licenses.txt. This is factory conversion, not arbitrary user-file import.

Original master Amp, all raw modulation curves, seed/FM/sample pitch and scalars
outside frozen knob ranges are retained internally. Editing Amp/Pitch uses the
existing16-knot representation. FCT1 source IDs/hashes survive native saves;
immutable source/projection manifests prevent accidental remapping. No new VST
parameters or Q-Link bindings; all4096 IDs and existing ranges remain. Decoded
sample budget11MiB accommodates the original16-sample kit within16MiB chunks.

Native sanitizers and full ARM64/QEMU suites pass: all109 states load,183 sound
entries compare first4096 samples to direct DSP oracles (max2.63e-5); source-hash
tampering rejects atomically; actual15,155,898-byte VST close/reopen and canonical
edits pass. Final118 XPL payloads/UID/version/path/hashes checked, installer
0 warnings. Hardware listening/browser acceptance pending; see0.0.29 test guide.

Next after acceptance: redesign including selected-WAV preview before Load.
General user.gkick/.gkit import, kit management and release tutorial remain later.
No formal Store benchmark or public release for this development build.

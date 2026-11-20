# Geonkick MPC roadmap

Target: Gen2 / aarch64, MPC OS 3.10.1+. Early builds tested by the user on Live III; new builds require separate validation.

1. **0.0.1: first playable prototype** — original DSP isolated; nine oscillators,
   three layers, global filter/drive, eight starting sounds, eight-voice playback,
   native pages/Q-Links, MPC program bank, Sounds tile and project state.
2. **Device validation** — installer, pad notes/velocity, layer/oscillator
   subpages, wheel and Q-Link synchronization, preset browser, Sounds tile,
   project save/reload, multiple instances. Measure live worker CPU during real
   time parameter edits, as well as the official audio-thread benchmark.
   - **0.0.2 implemented offline:** independent original filter and drive per
     oscillator, amp attack controls, nine FX subpages, 184 append-only
     parameters and migration of 0.0.1 project chunks. Native sanitizers and
     ARM64 shared-library/engine tests pass. No hardware claim yet.
   - **0.0.3 implemented offline:** visual SYNTH page with actual completed-hit
     waveform, amplitude/pitch envelope, editing controls and three oscillator
     modules. Native FilmStrip displays, UI-thread notifications, old-state
     migration, gray/green layout and actual-DSP offline preview. Device UI
     verification is the next gate; free point/Bezier editing remains pending.
   - **0.0.4:** 432 two-pixel waveform columns, finer vertical levels and corrected
     native text/captions. Approved update and app restart completed; on-screen
     plugin review is pending because MPC restarted into the track-type selector.
   - **0.0.5:** fine 5-cent frequency steps, millisecond timing, corrected cropped
     envelopes/decay length, conditional OSC controls and scoped captions. Native
     and ARM64 offline tests; hardware acceptance remains required.
   - **0.0.6:** asynchronous UI notifications outside the rendering thread,
     separate view/audio generations, exact cached audio on view switches and
     smoothed output gain. Slow-UI regression passes in an isolated Live III
     host; MPC-app listening/interaction acceptance is still required.
   - **0.0.7 candidate:** derived-display automation disabled, curve feedback
     deferred during gestures, synthesis requests coalesced and intermediate
     results published. Native/ARM64 regression tests pass. Verify graph repaint
     and real edit-time CPU/audio before claiming the issue fixed.
   - **0.0.8 candidate:** rounded fine-input boundary fix and shared graph
     invalidation replacing column notifications. Native/ARM64 checks pass;
     actual MPC redraw, hidden-page behavior and page return are experimental
     until tested. No reliable native current-tab callback is established.
   - **0.0.9 candidate:** shared refresh retained with one quiet-time host rebind;
     transparent larger horizontal slider targets. Temporary real-input
     observation for the remaining jumps; real MPC acceptance still required.
   - **0.0.10 candidate:** actual accumulated input stream reproduced, fine
     deltas tracked without the absolute-jump cutoff, gesture writes serialized
     and synchronous self-echo ignored. 0.0.9 graph/audio improvement confirmed
     by user; fine-control acceptance for 0.0.10 remains pending.
   - **0.0.11 candidate:** direct continuous touch handles, fine Q-Links/wheel
     and shared physical values; user confirmed the changes work.
   - **0.0.12 prepared locally:** native five-point Amp/Pitch editor for all nine
     oscillators, Simple/Points modes, curve copying, fine/direct point entry and
     GKMP4 state. Not installed at the user’s request; native gesture/geometry
     acceptance remains pending.
3. **Sound editing parity** — original arbitrary and Bezier envelopes,
   full useful filter range, sample loading, external preset import,
   graphical point editing and preview/export. First prototype: native fixed
   five-level/four-time envelope for selected oscillator amplitude/pitch,
   explicit custom-envelope state/migration and safe point/time ordering.
   Variable insertion/deletion and cubic curvature are implemented in 0.0.17 below;
   freely draggable desktop Bezier handles still need further UI work.
   See [feasibility](docs/ENVELOPE-EDITOR.md).
4. **Kit architecture** — sixteen independent drums, pad/key/channel mapping,
   per-drum choke/mute/solo, kit persistence, practical navigation. Decide output
   routing within the MPC VST host's actual capabilities; do not promise desktop
   multichannel parity before verification.
5. **Release** — visual refinement, public fork and source review, reproducible
   build workflow, exact hardware-tested release, benchmark evidence, catalog PR.

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

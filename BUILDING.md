# Build from source

## Prepare dependencies on a clean machine

Python3 with Pillow, a C compiler, aarch64-linux-gnu-gcc, matching cross libc,
binutils and qemu-aarch64 are required. On Ubuntu24.04 these come from the
standard build-essential, python3-pil, gcc-aarch64-linux-gnu,
binutils-aarch64-linux-gnu and qemu-user packages. No MPC firmware/SDK dump is used.

```
python3 tools/prepare_dependencies.py
export MPC_VST="$PWD/build/deps/wrapper"
export MPC_RELEASE_TOOLS="$PWD/build/deps/release-tools"
python3 tools/test.py native
python3 tools/test.py aarch64
python3 tools/build.py skin
python3 tools/package.py
```

The helper checks out the pinned public framework and applies all 22 published
patches in order. Existing unrelated checkouts are not reset or modified. The
optional workstation toolchain reuse described below is not required by users.
When changing the patch set, use `GEONKICK_DEPS=build/deps-<revision>` to select
a fresh cache; an existing cache with a different receipt is never overwritten.


Pinned Geonkick source is committed under src/dsp. No upstream checkout is
needed at build time. The upstream desktop CMake requires CMake 3.31, C++23,
Redkite and desktop-format dependencies; this C11 DSP-only port does not use it.

Build dependency: neighbouring open-mpc/mpc-vst-plugins at
`ef05a26c6e59f1bfd3ef2023e6a9d8b371b61310`, with
patches/mpc-float32.patch, patches/mpc-fine-int-sweep.patch and
patches/mpc-async-host-notify.patch, then
patches/mpc-display-parameter-budget.patch and
patches/mpc-coalesced-graph-and-fine-input.patch, then
patches/mpc-graph-rebind-and-input-observation.patch and
patches/mpc-accumulated-fine-input.patch, followed by
patches/mpc-canonical-continuous-echo.patch and finally
patches/mpc-envelope-state-capacity.patch, then
patches/mpc-idle-action-release.patch, then
patches/mpc-filter-control-feedback.patch and patches/mpc-control-context.patch, then patches/mpc-sample-dynamic-units.patch, patches/mpc-native-display-context.patch,
patches/mpc-ui-only-inputs.patch and patches/mpc-engine-program-bank.patch, followed by
patches/mpc-async-revision-cache.patch and patches/mpc-bounded-ui-feedback.patch,
then patches/mpc-numeric-parameter-read.patch, patches/mpc-indexed-popup-feedback.patch
and patches/mpc-readonly-input-fastpath.patch and patches/mpc-lazy-input-context.patch, in that order. The wrapper is not copied into this repository.
Release tooling: open-mpc/mpc-vst-plugins at
`bcc8242896f6948b2405014e8443c91941c4ded0` in the neighbouring
`mpc-vst-plugins-release-tools-20261122` checkout. This revision includes explicit VST2 plugin
format metadata and the current install/uninstall templates. The default
release-tools directory and exact pin are recorded in release.json.
`MPC_RELEASE_TOOLS` may select an equivalent checkout at that exact pin.

Requires Python 3 with Pillow, gcc, aarch64-linux-gnu-gcc 13 and its matching
ARM64 sysroot, binutils and qemu-aarch64. `MPC_VST`, `MPC_TOOLCHAIN` (the toolchain's
usr directory), `AARCH64_CC` and `QEMU_AARCH64` can override defaults. The current
workstation reuses the previously unpacked local JC303 toolchain; it is excluded
from source control and from installers. The compiler's glibc must be <=2.39.

```
python3 tools/build.py native
python3 tools/test.py native
python3 tools/test.py aarch64
python3 tools/build.py skin
python3 tools/package.py
```

The native tests load the actual VST shared library under ASan/UBSan/leak
detection, then check the engine against an independent signal oracle and
exercise asynchronous rendering. ARM64 runs the shared-library host and engine
tests under QEMU. QEMU is not a hardware test or CPU benchmark.
Version 0.0.2 adds 81 controls after the frozen original 103. Version 0.0.3 adds
two stored view controls and 288 derived display values after those 184.
GKMP3 saves 186 controls; GKMP1/GKMP2 migrate their 103/184 controls with default
view settings. Display values are neither stored nor writable.

tools/spec.py generates params.json, src/param_defs.h and layout.conf. Indices in
params.lock are frozen from 0.0.1 onward; append new parameters. An edit to
generated files must also update the generator. Skin finishing computes Q-Link
bounds from native controls, including labels, and renames the browser default
to Init. All build products, previews, logs and receipts stay under build/.

tools/plot_assets.py creates shared native display strips; finish_skin.py adds
the read-only plot components and namespaces compact selectors so detailed OSC
pages cannot overwrite their smaller SYNTH counterparts. No Chromium dependency
is needed. tools/render_ui.py renders a composite with actual native VST values
and checks host display notifications. Build native and skin first; invoke it
with Pillow available, as build.run does. See UI-DESIGN.md for details.

Packaging uses official release.py, verifies the test receipts and validates
the result with catalog_check.py. The ZIP contains only installation material,
including install/uninstall scripts, manifests, checksums and license notices.
Corresponding adapted source is published at
https://github.com/303inmyheart/OpenMPC-Geonkick on branch mpc-gen2. Each public
binary release is paired with its source tag and exact-binary benchmark.

0.0.4 preserves the first 474 VST entries and appends finer display columns
(total 1482). The 186 stored controls and GKMP3 state are unchanged. Native
caption pixels are generated per page; live numeric fonts use the MPC's observed
font-height conversion. The offline renderer compensates for the same conversion.

0.0.5 preserves all 1482 previously released VST indices and appends 46 unit
adapters plus one derived visibility selector (1529 total). Fine frequency uses
5-cent integer positions relative to 55 Hz, with physical endpoints 20/12000 Hz.
Legacy linear Hz/seconds parameters and all 186 stored values remain unchanged.
The second wrapper patch routes sub-step continuous drags through direction
history instead of counting against the already-rounded value. Repeated clamped
edge events still count individually. Tests cover both event rates, reversal,
continuous sweeps, endpoint clamping, host echoes and old frequency automation.

0.0.6 opts into ASYNC_HOST_NOTIFY (requires DISPLAY_REV_THREAD). Audio-side
UI notifications become per-parameter atomic pending flags. The normal-priority
display thread sends current values, coalesces repeated writes, and bounds each
notification pass to about 5 ms between host calls. A single blocked host call
can exceed that budget, but cannot hold up audio via this notification path.
The AEffect is fully initialized before the reporter starts; opting in fails
initialization if its thread cannot be created. Legacy synchronous mode compiles
unchanged. Shared host assertions are preserved; the port adds 30 ms after each
run() batch to await asynchronous callbacks instead of assuming delivery inside
processReplacing(). The dedicated slow-UI regression uses the actual .so and
asserts zero automate/update-display callbacks on its rendering thread.

Audio/view generations are separate. The worker retains a cache reference so a
view-only job can reuse exact audio instead of rerunning oscillators/effects.
Layers at zero gain do not execute their oscillators. Output/tune/keytrack/
playback do not schedule synthesis. A diagnostic synthesis
counter checks this in engine tests; it is not a stored or VST parameter.
OUTPUT uses a 5 ms smoothing time constant. Stored controls/parameter indices
and upstream DSP sources remain unchanged from 0.0.5.

0.0.7 enables LIVE_PARAMS_READONLY for the generated sorted PLUG_LIVE list.
Those 1297 derived values return false for effCanBeAutomated and ignore host
writes early, retaining their indices and get/display behavior. Original
frequency automation remains available. GRAPH_UPDATE_IDLE_MS=180 defers only
viz_* scans/notifications during parameter edits, then delivers current values
once quiet, even if display_rev did not change again. Exact stepped feedback
and non-graph control updates are not deferred. The new actual-library test
checks metadata, host echoes, zero graph callbacks during 90 edits, and refresh
after the gesture. Native sanitizers and ARM64 QEMU pass.

The synthesis worker starts at most one audio job every 33 ms, choosing the
latest snapshot at each start. It finishes and publishes that coherent snapshot
even if a newer edit arrives, then handles the newer state on the next job.
This avoids perpetual cancellation during a continuous gesture. render_pending
remains true until the current audio/view generations catch up. No state or
parameter indices changed. No candidate was installed without new approval.

0.0.8 enables FINE_INPUT_CONTINUOUS for nudge_pct integer controls. A small
non-absolute input no longer becomes an absolute position merely because its
floating value lands on an integer. Explicit MIDI/NRPN absolute writes and the
original physical-Hz parameter retain their previous meaning. Touch/wheel fine
controls disableCoarseDataWheel in their native definition. Existing continuous
sweep and boundary tests remain; new rounded-input tests cover the full pitch
range plus exact-grid millisecond controls.

GRAPH_INVALIDATION_ONLY skips viz_* host notifications entirely. A new appended
readonly graph_revision (index 1529, modulo 1024) changes when a completed cache
is adopted. SYNTH plot widgets bind their additionalInvalidatingHandles to that
shared parameter. The existing quiet interval also defers its notification.
The test host checks the signal, directly reads actual per-column values and
asserts that no per-column callback occurs. It cannot validate the proprietary
MPC skin invalidation implementation. Pending device checks: redraw after an
edit, no repaint of hidden pages, current graph on page return, and audible
redraw-time glitches. This is experimental; do not claim automatic page tracking.

No verified current-tab callback was found in the VST wrapper or the 110
read-only stock-skin references. EffEditOpen/Close describes custom editors,
not the native MPC tab. The design avoids guessing visibility from parameter
reads. JUCE 7's VSTParameter::getValue directly queries the loaded plugin
(https://github.com/juce-framework/JUCE/blob/7.0.12/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp);
using a shared invalidation handle is our MPC-specific hypothesis, not a
behavior guaranteed by JUCE. Stored state remains 186 values; 1530 VST entries.

0.0.9 keeps GRAPH_INVALIDATION_ONLY and adds GRAPH_REBIND_ON_REFRESH: one
UpdateDisplay on the existing non-audio reporter after shared revision delivery,
following the quiet interval. No periodic unconditional rebuilds or per-column
notifications are restored. This is a response to failed real MPC repaint in
0.0.8, and must be checked in the native host, including preset-browser behavior.

Horizontal slider filmstrips receive transparent padding, preserving every
original visible frame pixel. Their native Knob bounds include that padding;
controls and captions retain their global painted positions. Upward padding is
limited by nearby controls to prevent overlaps. The hit Knob is last in its
component so captions do not swallow its expanded drag area. Main envelope
sliders are 64 px high; tightly spaced sliders use smaller safe padding.

Temporary input observation is compiled only for this development investigation:
WRAP_TRACE records incoming and accepted values for fine controls and original
oscillator frequency controls. The producer only writes a bounded atomic ring;
a SCHED_OTHER writer handles file I/O. It activates only when /proc/self/exe is
named MPC, writes at most about 1 MiB per library load under its own /tmp prefix,
and is joined on unload. Actual test hosts leave it inactive. A test executable
named MPC exercised logger lifecycle under ASan; its own files were removed.
Remove this diagnostic after capturing the device input failure. Never publish
its raw device logs or include them in installers.

0.0.10 opts into FINE_TRACK_RAW_INPUT for nudge_pct integer controls. The real
MPC input stream can accumulate by 1/128 or 1/100 while the accepted fine value
moves by one integer. Compare consecutive raw inputs for direction/size instead
of using their growing distance from the accepted value. Sub-step absolute
sweeps keep settle() direction handling; large intentional moves remain direct;
explicit MIDI/NRPN absolute writes round to the requested position. Repeated
clamped endpoint events count individually. Old physical-Hz automation is intact.

The opt-in control pipeline is serialized by the existing recursive priority-
inheriting engine lock, including gesture history and final accepted values.
Host callbacks remain outside that lock. Synchronous echoes of our own active
notification are ignored using a saved/restored thread-local report context;
this also prevents an old echo from overwriting a newer concurrent user input.
The bounded input diagnostic remains enabled for hardware acceptance with a new
per-process /tmp prefix. Stored state, parameter indices and the working 0.0.9
shared graph refresh/touch geometry are unchanged.

0.0.11 appends 46 continuous *_touch adapters (1576 parameters total). The 186
stored physical controls and all 1530 previous indices/ranges/defaults stay
unchanged. Frequency touch uses log(Hz/20)/log(600); time/density touch uses the
original physical min/max. Fine adapters retain cents/ms/percent steps. Echoes
within normalized float formatting precision do not quantize restored physical
values. Edits mark only the counterpart alias for host refresh, resetting fine
input history after a touch edit without resetting it on each fine-input step.

Skin parent Data/action/label bindings and Q-Link maps retain the fine index.
Editable child Knob uses a separate named Touch handle remapped to its continuous
adapter. The same padded strips and pixel bounds remain. Actual VST tests cover
all 46 direct touch adapters, unrounded pitch, mirror readback, fine handover
and old Hz echoes. Structural checks validate the split on 265 widget instances;
Q-Links.json is byte-identical to 0.0.10. Named-handle behavior and visual
stability still require MPC acceptance. See docs/ENVELOPE-EDITOR.md for the
separate point-editor investigation; it is not part of this implementation.

Installer output stays under build/releases by default; no Desktop copy.
The --desktop option is an explicit opt-in for a later user request.


0.0.12 adds src/point_editor.h and 199 stored fields, preserving all earlier VST
indices. GKMP4 migrates GKMP1/2/3 as simple envelopes. The ENVELOPE native Pitch
component uses five level/four duration handles for both Amp and Pitch; schema
provenance, scaling and limits are documented in docs/ENVELOPE-EDITOR.md.
The temporary WRAP_TRACE/input_trace.c diagnostic is no longer compiled.
Tests include point-envelope DSP, state migration and actual-VST fine/direct
input feedback. Native geometry/gestures remain a hardware acceptance gate.


0.0.14 runs tools/navigation.py after skin finishing. It remaps the final native
pages to six keys, alternates each oscillator/FX pair, rebuilds rotary-only
RowTopDown Q-Link groups and complete focus bounds, and adds small title banners.
Touch-only controls are cloned per placed control; shared knob definitions and
the existing direct/fine handles remain intact. The layout's first qlinks seed
is deliberately unchanged to preserve existing MIDI CC assignments. Do not edit
only layout.conf: tools/spec.py regenerates it. check_navigation.py and
check_point_skin.py run during skin builds; render_ui.py creates offline images.
edge_test.c checks signal continuity/termination without relying on UI rendering.


0.0.15 runs skin_design.py after navigation and before the existing skin checks.
It generates original 3x raster panels, rotary filmstrips, native value backdrops,
pill/selector styling and headers without changing control bounds or inputs.
finish_skin.py reuses its segment_image helper; tile.py uses the same palette.
plot_assets.py preserves all strip sizes/frame counts while changing colours.
render_ui.py paints native backgroundData and envelope colours for faithful
layout review. All assets remain generated from source; no desktop screenshots,
manufacturer artwork or new runtime drawing engine is included.


0.0.16 opts into CANONICAL_CONTINUOUS_ECHO in the final wrapper patch. An exact
get/set echo of an ordinary continuous normalized value is a no-op before %g
conversion. That avoids six-digit physical rounding selecting the echoed knob's
oscillator. Fine-relative controls, switches, momentary actions, step/popup
handles and real changed writes retain the original path. Other ports do not
opt in and are unaffected. The full documented wrapper patch sequence was
checked against the clean pinned base. focus_echo_test.c exercises all nine
Amp/Pitch contexts, stopped and rendering, including async host rebinds and
preservation of fractional physical state. The same test fails on 0.0.15.


0.0.17 appends variable-envelope storage and touch proxies after all previous
1782 VST indices. GKMP5 stores 1521 values; GKMP4 still consumes exactly 385.
ENGINE_CHUNK_CAPACITY=65536 opts this port into a larger per-instance state buffer;
the shared wrapper defaults to 8192 for other ports. The final capacity patch is
included in patches/. free_chunk_test.c tests a >8 KiB actual shared-library
chunk, restart/restore, asymmetric Bezier handles and exact native proxy echoes.

src/free_envelope.h owns up to 16 points per curve, independent cubic control
ordinates and De Casteljau subdivision. The worker bakes each used free curve
into 4097 normalized samples; upstream synthesis interpolates those samples in
constant time instead of searching variable point lists or inverting cubics per
sample. No worker table lookup/allocation moves into the host audio callback.
An immutable generated hash index replaces repeated linear scans through the
larger stored parameter table. Storage channels are host-readonly and edits
notify only the small proxy set, not all curve fields. The quiet-time graph
refresh continues to use the existing shared invalidation.

The native Pitch Envelope has five grips. Its line is transparent; readonly
viz_e columns draw the complete actual-DSP curve above the existing waveform.
Virtual edit_l0..4/edit_t1..4 map endpoints and three banked interior points.
Two to four points use coincident inert placeholder grips at the right endpoint;
their appearance and actual native drag semantics require device acceptance.
Raw adjacent duration writes retain one proxy weight vector between events,
preventing normalization drift of untouched points. Actual point order is
clamped against hidden neighbors; endpoints cannot change their time.

free_envelope_test.c covers all 18 contexts, 2..16 point bounds, cubic insertion
sound preservation, raw paired time events, endpoint safety, malformed-state
atomic rejection, GKMP4 migration, original parameter automation and Copy Simple.
This remains an offline/QEMU regression suite, not a hardware CPU benchmark.


0.0.18 replaces the five-stage Pitch EG in the skin with native XYPad. Live III
screenshots showed that the native Pitch widget uses its own stage spacing;
normalizing four weights in our offline renderer had masked the mismatch.
Do not restore that assumed geometry. The selected XY pad and sixteen readonly
marker pads share one rectangle. Their normalised native puck centers encode
the common18px inset: x=(18+1064*time)/1100, y=(18+134*level)/170. The curve
strip occupies136px, with its134px line travel matching that inset exactly.
Source tools/editor_geometry.py is shared by skin generation and geometry checks.
C regressions check all points in all18 contexts and actual VST XY echo safety.
The two-to-four-point placeholder-grip limitation is removed.

IDLE_ACTION_RELEASE opts into a30ms normal-priority feedback deadline for each
momentary input, including highs that debounce before dispatch. A stopped MPC
need not call the audio callback for a button to reset. It changes only cached
UI feedback; engine actions remain once per valid tap. XY_POSITION_FEEDBACK
returns constrained selected-point positions to the host from the same existing
notification thread. Both options are off for unrelated ports. No new audio
thread UI call, graph callback burst or synthesis wait is added.
idle_action_test.c reproduces the stopped-host cache, repeated taps and the
finger-up high pulse after the first reset, and echoes notifications back to the
actual shared VST. No formal Store benchmark is implied by these checks.


0.0.19 appends30 physical filter fields and40 fine/direct proxies (3040 VST
parameters, old2970 frozen). GKMP6 stores1551 values. GKMP5 still consumes exactly
1521 values and every older header migrates all10 models to CLASSIC; WIDE fields
are seeded from the old frequency and bounded equivalent Q. New Init defaults
to WIDE; the two starting hat presets explicitly stay CLASSIC. Each model keeps
its own frequency/resonance fields. filter_controls.h maps continuous Touch and
fine5-cent /0.1-percent controls to that model without quantizing saved physical
values on echoes. Existing raw legacy changes also propagate their frequency /
bounded equivalent Q to WIDE when active; legacy projects themselves stay CLASSIC.

filter.c retains the original CLASSIC arithmetic. WIDE uses an original C
implementation of the trapezoidal state-variable equations, with double state /
coefficient precision and cached tan-based coefficients for constant envelopes.
Mathematical reference: Andrew Simper, Cytomic,2013,
https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf . No third-party
implementation source/library was copied or bundled. License remains GPL-3.0-or-later.
Band-pass is normalized by damping to unit gain at its centre; LP/HP resonance
still intentionally increases their peak, and the existing final limiter remains.

filter_test.c compares162 cutoff/Q/type cases against an independently derived
bilinear biquad, checks exact CLASSIC recurrence, all10 paths, old-state migration
and continuity of a held cached hit during new synthesis. filter_controls_test.c
uses the actual shared VST for Touch, accumulated raw Q-Link steps, echoes and
idle CLASSIC clamp feedback. Optional filter_compat_test.c takes old/new shared
libraries; against the archived0.0.18 ARM64 binary,30 old-project fixtures yield
maximum sample difference0. No firmware binaries or private data are fixtures.
No new locks/allocations/synthesis/UI calls are added to the host audio callback.


The0.0.20 Humanizer implementation is retired in0.0.21. Its runtime scheduler,
controls and stored fields are removed; the last three old state tokens are
consumed only for migration, and seven old VST parameter slots are inert.
The original upstream DSP dependency still contains its unused implementation.

0.0.21 uses1551 physical values per pad across16 pads, GKMP8 kit state and3053 VST
parameters. Existing3040 identities/ranges are unchanged; the seven Humanizer
slots are retired/live/legacy/nonautomatable, not repurposed. Six new kit controls
are appended. Legacy GKMP7 still consumes1554 values and discards the final3.
State is a full first selected row followed by `KIT1 mode base selected output`
and15 ascending other rows: `D` is Init/default, `P` prefixes1551 stored values.
The entire structure is validated before publication. Wrapper chunk capacity1MiB.

One shared worker/DSP,36 preallocated audio slots,16 voices. Initial identical
Init sounds share one immutable slot with per-pad current/worker references.
Per-pad atomic revision counters plus the existing seqlock avoid mixing states
while selecting/editing. Slot owners, ready/current adoption and held-note refs
remain bounded. Selection/playback-only edits do not resynthesize sounds.
No new audio-thread allocations, file IO, waits or host UI calls are introduced.

`CONTROL_CONTEXT_REV` is opt-in: the wrapper checks the engine's context token
before a host control write, clearing accumulated raw positions only when the
edited instrument/preset changed. Apply the new patch last; other ports do not
enable the behavior. Native/actual ARM64 context tests exercise the first fine
step immediately after selection before any asynchronous notification/rebind.

Additional actual VST capacity check:all16 sounds ×18 curves ×16 points;
238482-byte full-kit chunk survives complete close/reopen natively and onARM64/QEMU.


0.0.22 appends3 VST entries (3056 total, first3053 frozen):kit_mapping, kit_learn,
kit_learn_status. Fixed16-int capture stage and learned map; duplicate notes do
not advance, only a complete unique stage commits. Preview calls hit_pad directly,
never feeding its note into capture. Normal note routing uses at most16 integer
comparisons. Sound values/DSP/worker and existing context hook are unchanged.
GKMP9 keeps the GKMP8 body and appends `MAP1 mapping note0 ... note15`. Validate
all notes0..127, integer/unique, before publishing any part of a state. Earlier
kits default the saved table to their original chromatic base. Learning UI flags
and partial staging are not serialized. New KIT footer and note-map controls are
Touch only; header/grid/rotary bounds remain checked.


0.0.23 retains all previous3056 VST entries and1551 stored floats per pad;4413
parameters total after sample controls and3×432 readonly layer curves. The new
viz_l* fields are graph parameters: no per-column notifications. Sound tab
component bindings choose their own layer data, so no host tab-event workaround
or synthesis is required. Matching local focus/context drives overview sliders.

Sample assets are immutable float32 plus pre-encoded base64/name. One worker does
folder IO and WAVE decoding. A fixed request ring, outstanding-reply counter and
per-slot generation token coordinate UI adoption. Source asset pointers are read
inside the existing seqlock snapshot; reader epochs and retired stacks defer
freeing to worker/normal display. Current/held audio voices only read rendered
cache slots, never file/sample assets. Preset/clear actions retire, never free on
audio. Borrowed gkick_buffer views are detached before owning DSP teardown.
Only source buffers enable end fading/cached exact pitch factors; ordinary DSP
buffers default those additions off. Constant sample pitch needs only a2-point
shift table; arbitrary Pitch curves keep the existing baked table.

GKMP9 optionally appends `VIEW1` plus16×3 layer-focus indices, then `SMP1 records`
and unique `(pad osc source count hexname base64)` records. Empty assets use
count0 and `- -`; decoded PCM is finite/full-scale float32,2..176400 frames.
Validate all records/byte limits before publishing any state. Names contain no
absolute source paths.11MiB PCM budget +16MiB wrapper chunk capacity. No sample
content/dependency is included in the installer. Newest wrapper patch enables
DYNAMIC_PARAM_UNITS only here; fine/direct frequency handles report semitones
when they control a sample and otherwise retain their old Hz metadata.

0.0.24 sets MODULE_SUBDIR to "." so the existing wrapper resolves the plugin
directory dynamically. The worker saves the chosen sample default folder via
private temporary file + atomic rename to `.sample-folder` there. The installer
manifest declares that file as user_data so upgrades retain it. It is generated
only by the user action and never bundled or copied into project/preset state.
Layer waveform peaks are cached during existing synthesis and copied for view-only
refreshes, not recalculated by reading a tab.

Run native and ARM64 builds sequentially: preparation regenerates shared
parameter headers, so concurrent compiler invocations are not independent.

0.0.25 adds a14th opt-in wrapper patch, mpc-native-display-context.patch, after
mpc-sample-dynamic-units.patch. UI_DISPLAY_CONTEXT calls the port's weak hook only
from effGetParamDisplay formatting. Ordinary getParameter, metadata names,
reporter scans and engine reads do not select pages. Visible text markers choose
one cached SOUND layer or the mixed/editor view. No sample loop or file IO runs
in this hook. A context change invalidates the graph once; repeated labels are
idempotent. Actual native page-formatting behavior needs hardware acceptance.

Native skin handles must be0..4095. All physical controls, first3111 descriptors,
three focus/context IDs and GKMP9 project chunks stay fixed. Only derived display
slots are relocked: shared432-column positive/negative layer strips, four native
text context markers, and Set Default at4095. Old display-only values were never
automatable or saved. Platform CLI builds/tests now serialize with flock to keep
shared generated headers stable throughout library and test-host compilation.

0.0.26 refines UI_DISPLAY_CONTEXT in the engine only: the formatting hook queues
its latest context and timestamps only a changed request. display_rev adopts it
after100ms stability, incrementing the graph revision only for a real committed
change. This avoids publishing first-tab intermediates during MPC's native
rebind. Hook requests themselves never trigger resynthesis or immediate rebinds.
The14-patch wrapper stack is unchanged. The replay fixture exercises the real
shared library with a host that reproduces the measured formatting order.

0.0.27 disables UI_DISPLAY_CONTEXT (old patch14 may remain applied but is inactive).
The engine has no formatter hook, pending context or timer. sound_layer directly
reads plot_osc/3; setting it restores the per-layer remembered oscillator. Shared
waveform reads use this same layer and the original envelope uses plot_osc.
Seven unused display slots3985..3991 become the selector and6 fine/level overview
proxies; first3985 descriptors and3992..4095 stay exact. GP1551/GKMP9 unchanged.
Proxy set/get/display/unit calls resolve to current layer's canonical oscillator
fields; fine edits enqueue canonical fine feedback. Direct frequency Touch clears
control_context so subsequent shared fine input does not reuse an old coordinate.

Opt-in patch15 mpc-ui-only-inputs.patch adds a weak UI-only predicate for
canBeAutomated. It prevents recording a context-dependent proxy index; direct
setParameter still edits canonical parameters and their feedback remains. Detail
parameters retain their original IDs/automation. Other ports do not opt in.
Native automation recording acceptance is a hardware check, not proven by QEMU.

Skin build: generate/style/check original panels, then sound_overview.py flattens
and masks them with IndexedEnabling/sound_layer, inserts the three native Touch
buttons and remaps six rotary Data bindings/Q-Links. Detail Touch handles stay
physical. check_sound_overview.py verifies the final deliverable; package.py
requires that check. Original intermediate navigation checker still validates
unchanged detail pages. Captions/formatting never select a layer.


## Native MPC presets (0.0.28)

The skin build first compiles the optimized native plugin. tools/native_presets.py
loads it through a minimal VST host, exports eight actual complete GKMP9 chunks
and builds a16-pad kit using the preserved program bank. tools/xpl.py from the
external wrapper encodes these authentic chunks into standard native.xpl files.
The generated manifest and tools/check_native_presets.py validate the deliverable
files and their payloads. The latter is required by tools/package.py.

These are the port's eight starter recipes, not the upstream factory library.
Original.gkick/.gkit import and missing engine parity are deferred. All native
presets are complete-instance snapshots; no new scoped chunk format is used.
Existing old program/parameter identities remain. Native Save preserves the
full kit, learned map and embedded sample data using existing GKMP9.

Portable XPL PLUGIN.file values contain %payload-path%, replaced by install.sh.
Direct plugin-only deployment must make that same replacement before computing
the deployed file hashes, then validate the installed absolute plugin path/UID.
The downloadable installer remains portable and its audio binary is unchanged.


## Original factory bank (0.0.29)

tools/factory_bank.py verifies all nine archives in factory/original against
factory/sources.json, retaining original content/licences. It generates a readonly
C synthesis bank and109 native state fixtures. factory/index.json freezes each
source identity/hash and factory/projection.json freezes its canonical control
projection; changing either requires an explicit state-migration design. No new
VST parameter IDs are introduced. The worker snapshot adds one factory ID.

An optional FCT1 extension after VIEW1/SMP1 records each imported pad's ID/hash.
Unknown/tampered identities reject the entire load before publication. Native
Save keeps canonical edits plus these identities and embedded samples. Loading
old states or Init clears absent identities. Selected-pad reset clears only that
pad's import identity. No file parsing, allocation or mutex is added to audio.

The worker preserves original master Amp, raw frequency/log mode, oscillator
seed/FM coefficients, sample pitch and filter/drive/volume envelopes. Existing
canonical controls override their corresponding original scalar on edit; editing
an Amp/Pitch curve activates the existing sixteen-knot representation. Original
out-of-range scalars are retained until the corresponding control is edited.
Native browser files are generated by the existing XPL pipeline with all source
READMEs/licences. Arbitrary user.gkick/.gkit import is not implemented here.

The decoded sample budget is11MiB, allowing the original sixteen-sample AVL kit
within the existing16MiB wrapper chunk budget. Original source buffers are bounded
to four seconds, as in the original44.1kHz engine. The largest native VST fixture
is15,155,898 bytes. No formal Store benchmark is claimed for this dev build.


## Native MPC program menu integration (0.0.30)

Apply patch16 patches/mpc-engine-program-bank.patch after patch15. It is guarded
by ENGINE_PROGRAM_COUNT, enabled only for this port. Other ports keep the existing
program logic. The wrapper queries __program_index and __program_name:N and sends
__program_load; no mpc_engine_t ABI or VST parameter metadata changes. All118 menu
names fit the23-byte VST name limit and are unique. Browser preset names match.

tools/program_bank.py generates readonly complete-state literals for appended
programs; choosing a program requires no runtime file access. Legacy indices0–7
retain their selected-pad meaning; index8 is the complete starter kit,9–117 the
original factory presets/kits. PRG1 saves the current bank identity alongside
FCT1/embedded samples. Old0.0.29 identities migrate by exact source-ID vectors.
Restored current-program re-selection preserves edits, as required by JUCE host
name refresh behavior. Program name queries never change sound state.

For a published release, measure the exact stripped installer binary first, then
use `python3 tools/package.py --with-benchmark` to attach its standard report.
The attachment requires a matching binary SHA-256 and qualifying PASS/WARN.
Normal local/CI packages omit benchmark claims: another compiler build must not
inherit measurements from a different binary, even with the same source version.

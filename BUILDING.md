# Local build

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
patches/mpc-accumulated-fine-input.patch applied, in that order. The wrapper is not copied into this repository.
Release tooling: open-mpc/mpc-vst-plugins at
`a2dd2b78a19c4cd46257c09eb9a485a155ab2968` in the neighbouring
mpc-vst-plugins-release-tools checkout.

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
The source-repository field is reserved for the intended future
303inmyheart/OpenMPC-Geonkick repository; publication is still pending.
Do not submit this prototype to the catalog or claim public source availability.

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

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
   Free point insertion/deletion and Bezier handles need further UI work.
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

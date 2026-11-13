# Visual synth interface — 0.0.3, 2026-11-13

The reference is Geonkick's own synth screen: a large waveform and envelope
area above three oscillator modules, with master controls beside them. The
first two MPC prototypes lacked this visual feedback. The new default SYNTH
page follows that hierarchy in an original gray/green layout.

- Green: actual completed synthesis buffer before velocity, playback tuning
  and final output gain. The 96 columns show positive/negative sample extrema,
  quantized to 64 display levels.
- Gray: the selected oscillator's amplitude or frequency-envelope shape,
  independently normalized to 0–1, from Geonkick's envelope evaluator.
- X-axis: the complete sound duration, shown by DURATION in seconds.
- Attack, decay, pitch drop and pitch time are editable below the graph.
- The lower row shows three oscillator modules. SYNTH subpages select layers
  1–3 and expose their layer gain; MIX contains other global controls.
- Nine focus buttons select the envelope. Editing an oscillator control also
  selects that oscillator's envelope; audio routing is independent of the view.

This is not the desktop editor or a real-time scope of mixed playing voices.
Point dragging, insertion/deletion and Bezier handles remain pending. Detail
pages remain available for per-oscillator effects and additional parameters.

Read-only inspection of 110 stock TUI skins on the available Live III found
Knob, Slider, Button, Label, Image, Meter and related native types. No stock
free-form Envelope/Graph/Canvas component was found. Wave-named entries were
component aliases, not a plotting API. This describes those skins; it does not
prove that MPC internals can never support a custom UI. No stock JSON or art
is committed, bundled or changed.

The prototype uses three shared native FilmStrip assets and 288 read-only VST
display values. Columns ignore touch. The synth worker prepares values with
each completed sound; normal audio playback does not calculate curves. A
normal-priority wrapper thread reports changes at 100 ms intervals without
periodic UpdateDisplay calls that would close the preset browser. A view switch
currently resynthesizes the sound before showing its new envelope; optimize
this if device tests show latency on long or complex sounds.

Native sanitizers and ARM64 QEMU tests compare columns with captured audio,
verify amplitude/pitch overlays, reject writes to graph values and refresh a
completed graph without calling render. tools/render_ui.py also exercises the
actual native VST's display notifications. Its build/ui-synth.png is an offline
composite using queried VST values and approximate DejaVu fonts, not a device
screenshot. Generic studio previews use an arbitrary shared frame and therefore
show solid graph blocks; use render_ui.py for an actual sound preview.

Hardware checks remain: frame selection, curve refresh while editing/idle,
preset browser, touch targets, Q-Link bounds, derived display channels in host
parameter lists, UI latency and CPU under real interaction. No graphical behavior
is claimed as device-verified yet. The user approved installing 0.0.3 and its
files have been updated and checksum-verified on the MPC. After separate user approval, the MPC application was restarted and returned
to running. Actual on-screen graph behavior and interaction testing remain pending.

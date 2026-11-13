# Visual synth interface — 0.0.4, 2026-11-13

The reference is Geonkick's own synth screen: a large waveform and envelope
area above three oscillator modules, with master controls beside them. The
first two MPC prototypes lacked this visual feedback. The new default SYNTH
page follows that hierarchy in an original gray/green layout.

- Green: actual completed synthesis buffer before velocity, playback tuning
  and final output gain. The 432 columns show positive/negative sample extrema,
  quantized to 97 waveform levels (one pixel per step). Envelope height uses
  85 levels; each shared strip stays under the 16384-pixel image-height limit.
- Gray: the selected oscillator's amplitude or frequency-envelope shape,
  independently normalized to 0–1, from Geonkick's envelope evaluator.
- X-axis: the complete sound duration, shown by LENGTH in milliseconds.
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

The prototype uses three shared native FilmStrip assets and 1296 read-only VST
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

## 0.0.4 display correction

A read-only capture of the actual 0.0.3 screen confirmed nine-pixel waveform
blocks and undersized native captions/values. Version 0.0.4 uses two-pixel
columns over the same 864-pixel plot width. Page captions are rendered at native
pixel resolution with the intended layout text; native live Value fonts are
increased by 1.6 to compensate for the actual MPC font-height units. Caption
images sit below controls so they cannot intercept touches. All 474 previous
parameter indices remain intact; extra derived display columns are appended.

The approved 0.0.4 build was installed, checksum-verified and the MPC app restarted.
The app returned to its track-type selector, so an after-update capture could
verify that it was running but could not verify the new plugin screen. No UI
input or project/instrument selection was performed to obtain a screenshot.
Device captures and old-plugin backups remain private, ignored build artifacts.

## 0.0.5 editing correction

Every selector now has size- and text-specific artwork. Captions are native-size
images inside each control definition, following its conditional visibility.
Tone/noise detail pages expose applicable controls, layer level is explicit,
and bank labels include layer identity. Fine Hz and millisecond adapters do not
change original project values or old parameter indices. The graph retains its
432-column resolution; this version does not claim an additional waveform
resolution increase. Actual-value previews cover SYNTH, tone OSC and noise OSC.

## 0.0.6 scheduling correction

Changing envelope focus/view now copies the held rendered sound and calculates
its display instead of rerunning synthesis. OUTPUT/tune/playback/key tracking
never schedule synthesis. UI feedback, including accepted stepped values and
MIDI/engine-driven changes, is delivered from the display thread; repeated
pending updates are coalesced to the current value. Curve changes no longer
request a full UpdateDisplay screen rebuild. This is not a new drawing engine;
waveform resolution and image assets retain the 0.0.5 format.

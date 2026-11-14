# Point-envelope editor — 0.0.13 prototype

The pinned original Geonkick UI adds points in `src/envelope_draw_area.cpp`
(double-click), moves selected points on mouse drag, and removes points through
its editing actions. `src/envelope.cpp` maintains time-sorted, normalized points
and separate Bezier control points. The vendored DSP already accepts point
lists through `gkick_envelope_set_points`; amplitude and frequency envelopes
are separate. The adapter retains those simple curves and now also stores five linear points
per amplitude/pitch envelope.

Original source: [Geonkick envelope editing](https://github.com/quamplex/geonkick/blob/462fed9bd8c48579a8819e7b360f8a2999b246fc/src/envelope_draw_area.cpp),
[point model](https://github.com/quamplex/geonkick/blob/462fed9bd8c48579a8819e7b360f8a2999b246fc/src/envelope.cpp).

The MPC native Envelope component is documented in the local MPC porting
reference as a draggable fixed five-level/four-time editor (Pitch type), tested
on a Live III in another port. This is a promising native interaction route;
0.0.12 implements its parameter bindings in Geonkick MPC, with two-dimensional dragging confirmed by the user on Live III.
The waveform overlay added in 0.0.13 still needs acceptance. The regular
VST skin does not currently give this adapter raw touch coordinates for a free
custom drawing area.

The current local prototype:

- Shows a selected oscillator's amplitude or pitch envelope on a separate ENVELOPE tab.
- Uses the native fixed-point editor, with large touch handles and continuous
  touch coordinates; point editing is Touch-only from 0.0.13.
- Adds explicit point-envelope storage and a versioned state migration, retaining
  the existing simple attack/decay mode and the eight starting sounds.
- Maps and validates point order, segment time, levels and pitch scale; avoid
  zero-length segments or accidental parameter resets while dragging.
- Feeds the existing bounded synthesis worker and the working coalesced graph
  update, without restoring per-column host notifications.

Freely adding/deleting an arbitrary number of points and dragging Bezier handles
like the original requires a further UI solution beyond the documented native
fixed-stage component. Do not promise desktop editing parity from this finding.
The editable curve controls amplitude/pitch over time; the green waveform shows
the resulting sound. The new editor and GKMP4 migration are part of 0.0.12; the preceding 0.0.11
Touch/Q-Link split remains intact.


## Controls and storage

Select layer/oscillator, AMP or PITCH, then POINTS. The first activation inserts
interpolated points into the existing simple curve until five points are present.
Switching back and forth preserves later custom edits. COPY SIMPLE explicitly
replaces custom points with the current simple controls. Editing attack/decay
or pitch-drop/time on the older pages returns the corresponding envelope to
Simple; the custom points remain available when Points is reselected.

Five level values use 0–1. Amp displays percent. Pitch displays Hz, mapped from
20 Hz to min(20 kHz, 16 × oscillator base frequency); changing base frequency
rescales the custom pitch curve. Time parameters are four positive weights
(minimum 0.0001), normalized to the current sound length. Segment Time displays
the resulting milliseconds. Changing one weight also changes the proportional
durations of the other segments. The first point is fixed at time zero; its
Segment Time readout is START / 0 ms and time edits there do nothing. The final
point is at the end of the sound. There are no crossing or zero-length segments.

Drag handles for direct entry, or choose POINT 1–5 and edit POINT VALUE /
SEGMENT TIME. Touch knobs are continuous. Point selection, values and Copy Simple have no
Q-Link assignment or hardware focus from 0.0.13. Fine aliases remain internally
for compatibility, not as current point-control Q-Link bindings. The curve uses the existing synthesis worker and shared
quiet-time graph refresh. The native editor geometry is host-rendered; offline
previews approximate it and cannot prove the actual touch behavior.

GKMP4 stores 385 values: the original 186 followed by 198 envelope fields
(mode, initialized flag, five levels, four time weights × 18) and point selection.
All 1576 earlier VST indices remain unchanged; the new total is 1782. GKMP1/2/3
states load with Simple mode and uninitialized custom curves. Saved custom
curves stay local to their oscillator and Amp/Pitch target.

Native component schema reference:
[MPC Plugin Builder SDK ui-reference.md](https://github.com/inmusic-software/mpc-plugin-builder-sdk/blob/febd2ca5bd9eb16c4a412837196e145566bc981b/docs/ui-reference.md#613-envelope-and-envelopeoverlay-advanced-test-on-hardware).
No proprietary SDK or stock artwork is bundled. The temporary input diagnostic
is disabled in 0.0.12. 0.0.12 was initially prepared locally, then installed at the user’s request.
The user confirmed point dragging works but requested simultaneous waveform
visibility. 0.0.13 adds the same completed mixed-sound waveform behind the
editable curve, using existing read-only columns and shared revision. Read-only
waveform widgets ignore touch; the native Envelope sits above them. Simple mode
also shows the waveform. Graph feedback still follows the 180 ms quiet interval.
Waveform time positions follow the full current sound length; native Envelope
geometry remains host-controlled and needs alignment checks on the device.

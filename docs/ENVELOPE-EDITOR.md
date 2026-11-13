# Point-envelope editor feasibility

The pinned original Geonkick UI adds points in `src/envelope_draw_area.cpp`
(double-click), moves selected points on mouse drag, and removes points through
its editing actions. `src/envelope.cpp` maintains time-sorted, normalized points
and separate Bezier control points. The vendored DSP already accepts point
lists through `gkick_envelope_set_points`; amplitude and frequency envelopes
are separate. Our adapter currently regenerates simple envelopes from its
attack/decay and pitch-drop/time controls, so arbitrary points are not stored.

Original source: [Geonkick envelope editing](https://github.com/quamplex/geonkick/blob/462fed9bd8c48579a8819e7b360f8a2999b246fc/src/envelope_draw_area.cpp),
[point model](https://github.com/quamplex/geonkick/blob/462fed9bd8c48579a8819e7b360f8a2999b246fc/src/envelope.cpp).

The MPC native Envelope component is documented in the local MPC porting
reference as a draggable fixed five-level/four-time editor (Pitch type), tested
on a Live III in another port. This is a promising native interaction route;
we have not yet implemented or hardware-tested it in Geonkick MPC. The regular
VST skin does not currently give this adapter raw touch coordinates for a free
custom drawing area.

A practical first prototype would:

- Show a selected oscillator's amplitude or pitch envelope over the sound view.
- Use the native fixed-point editor, with large touch handles and continuous
  touch coordinates; keep Q-Link entry precise for the selected point.
- Add explicit point-envelope storage and a versioned state migration, retaining
  the existing simple attack/decay mode and the eight starting sounds.
- Map and validate point order, segment time, levels and pitch scale; avoid
  zero-length segments or accidental parameter resets while dragging.
- Feed the existing bounded synthesis worker and the working coalesced graph
  update, without restoring per-column host notifications.

Freely adding/deleting an arbitrary number of points and dragging Bezier handles
like the original requires a further UI solution beyond the documented native
fixed-stage component. Do not promise desktop editing parity from this finding.
The editable curve controls amplitude/pitch over time; the green waveform shows
the resulting sound. Neither point editing nor an envelope-state migration is
implemented by the current touch/knob change.

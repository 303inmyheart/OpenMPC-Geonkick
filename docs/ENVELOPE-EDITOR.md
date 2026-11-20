# Variable Amp/Pitch envelopes — 0.0.17 local candidate

Every oscillator has independent amplitude and pitch curves. SIMPLE uses the
existing Attack/Decay or Pitch Drop/Time controls. POINTS retains custom edits;
COPY SIMPLE replaces the selected custom curve from those simple settings.
Changing the older simple controls returns the corresponding envelope to SIMPLE.

## Editing

1. Open ENVELOPE, choose L1/1–L3/3 and AMP or PITCH, then POINTS.
2. The teal waveform is the actual completed **mixed hit**; the amber line is
   the complete selected oscillator envelope. Both use the full sound duration.
3. Drag a native grip in two dimensions. Endpoints and up to three interior
   points are available at once. GRIP GROUP switches the interior set; POINT
   previous/next selects individual points, including those in another group.
   The status identifies the selected point and current count.
4. ADD POINT inserts halfway through the selected outgoing segment (or the last
   segment when the final endpoint is selected). Cubic subdivision preserves
   the existing curve. The new point becomes selected. Maximum: 16 points.
5. DELETE POINT removes the selection and joins the neighboring segments.
   Start/end cannot be deleted; minimum: two endpoints. Deleting a point can
   change the sound. There is currently no undo stack.
6. POINT LEVEL edits the selected value, POSITION its absolute time within the
   current sound. Neighbor order is protected. Start/end times stay at 0/end;
   their levels remain editable. Increasing LENGTH rescales time positions.
7. SEGMENT CURVE bends the selected **outgoing** segment. Zero is straight;
   positive bends upward, negative downward. The final point has no outgoing
   segment. This macro moves two cubic Bezier ordinates; it is not a pair of
   freely draggable desktop Bezier handles.

All point editing, including its knobs, is **touch only**, with no Q-Link or
hardware focus assignments. LENGTH retains its Q-Link. Existing non-editor
fine Q-Links/wheel and coarse continuous Touch paths are unchanged.

Graph feedback keeps the existing 180 ms quiet-time refresh. Native grips move
immediately; the completed curve/waveform follow the synthesis worker. No
per-column callback burst, UI file IO or synthesis wait is added to audio.

## Native UI limits and validation

The documented MPC native Pitch Envelope has five grips, not a free drawing
canvas. Its own line is transparent; readonly filmstrips show all segments,
including those outside the current grip group. Unused grips for sparse curves
coincide with the final endpoint and ignore edits. Actual sparse-grip appearance,
hit testing and drag behavior require Live III acceptance. Offline previews only
approximate the host's geometry. ADD uses a button; tapping an arbitrary empty
canvas coordinate to insert is not implemented.

First activation of a legacy POINTS curve keeps its existing five-point data.
Variable editing copies it lazily. GKMP5 stores 1521 values, including eighteen
2–16-point lists with independent Bezier control ordinates. The original 1782
VST indices/ranges/defaults remain unchanged. GKMP1/2/3/4 load without losing
their simple or original custom envelopes. New GKMP5 states require 0.0.17+.
The actual shared VST uses an opt-in 64 KiB state buffer; larger complete curves
are roundtrip-tested instead of relying only on direct engine calls.

Cubic evaluation and point-list edits happen off audio. The synthesis worker
bakes 4097 samples per active free curve, and upstream synthesis uses constant
lookup/interpolation. Linear legacy curves retain the old path until edited.
There are no unrestricted point-list allocations on the host audio callback.

Original desktop implementation: [Geonkick envelope editor](https://github.com/quamplex/geonkick/blob/462fed9bd8c48579a8819e7b360f8a2999b246fc/src/envelope_draw_area.cpp).
Native schema reference: [MPC Plugin Builder SDK envelope documentation](https://github.com/inmusic-software/mpc-plugin-builder-sdk/blob/febd2ca5bd9eb16c4a412837196e145566bc981b/docs/ui-reference.md#613-envelope-and-envelopeoverlay-advanced-test-on-hardware).
Neither an SDK nor manufacturer artwork is bundled.

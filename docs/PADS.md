# Hardware pad-note learning —0.0.22

The physical pad numbers and the MIDI notes received by a plugin are different
things. Pad Perform can provide scale/custom notes, so changing only a chromatic
starting note cannot cover every layout. See [Akai’s note-mapping guide](https://support.akaipro.com/en/support/solutions/articles/69000867733-akai-pro-mpc-series-how-to-remap-the-mpc-pads).

1. Stop the sequence and select the hardware bank you will use (e.g. Bank A).
2. Open MASTER → KIT and choose LEARN16 under LEARN HARDWARE PADS.
3. Play hardware pads01–16 once in numerical order, starting bottom-left and
   continuing across each row. The status identifies the next expected pad.
4. On completion, NOTE MAP becomes Learned and the status says16 PADS LEARNED.
   Each physical note now plays its matching sound even when notes are not
   chromatic, consecutive or increasing. Save the MPC project/native plugin preset.

Off cancels learning without replacing the previous note table. Partial capture
is not persisted. PLAY SELECTED PAD and zero-velocity/note-off/CC events do not
count as learned hardware notes. Repeated note-ons do not advance the slot.
If two physical pads send the same MIDI note, the plugin cannot identify them
individually: the status flags SAME NOTE. Choose a Pad Perform mapping with unique
notes (e.g. Chromatic) and start again, or press the next correct pad if you
accidentally repeated one. Changing the MPC’s note/scale/octave settings later
changes its output notes and requires matching/relearning the map.

Chromatic remains available; CHROMATIC START affects only that mapping. The
learned table remains stored when switching to Chromatic and back. Older GKMP8
kits retain their exact old routing/base; earlier GKMP1–7 remain Single. GKMP9
adds MAP1 + mode and16 unique MIDI notes to the whole-kit state. It is not readable
by0.0.21. Sounds and point envelopes are unchanged by learning.

# Pad instruments — Geonkick MPC0.0.22

Open **MASTER → KIT**. All sixteen sounds initially contain Init. Select01–16
with Touch, then edit using the existing SOUND/LAYER/ENVELOPE/MASTER pages. Their
header shows the selected pad. Selecting does not play it; PLAY SELECTED PAD
previews it. PRESETS and INIT SELECTED PAD affect only this one instrument.

**Single** is the default, preserving the previous chromatic behavior: every
MIDI note plays the selected sound, with optional Key Track. **16 Pads** maps
the sixteen sounds using the selected map. With Chromatic, CHROMATIC START is the
first assigned note, default36 (36–51); with Learned, the saved individual notes are used.
Read the MIDI number rather than relying on host octave naming. Notes outside
the assigned set are ignored. In kit mode, notes select sounds without pitch
transposition; Tune remains independent for each sound. Channel is not filtered.
Note input triggers a sound without changing the sound selected for editing.

OUTPUT on the sound pages is per sound. KIT OUTPUT controls the mixed instance.
Poly/Cut/Gate are per sound; Cut retriggers only that sound, and a Gate note-off
still reaches its voice after editing another pad. CC120/123 clear every voice.
Sixteen playback voices are shared; overflow steals the oldest with a short
smooth tail. All sounds feed the existing stereo output.

Save the MPC project or a native MPC plugin preset to store the entire kit.
GKMP9 includes all sound parameters/curves, selection and routing/output settings.
Earlier GKMP1–7 states load into pad01 in Single mode, other pads reset to Init.
Older builds cannot read the new kit state. The original.gkick/.gkit import is
not implemented in this build; it is the following task. Native MPC saving is
used rather than implementing a competing save browser.

Q-Links control rotary knobs only. Selection/routing are Touch. Existing shared
host automation controls address the currently edited sound, not sixteen new
independent parameter sets; dedicated per-pad automation is deferred.

Pad layout matches the MPC:01 bottom-left,04 bottom-right,13 top-left,16 top-right.

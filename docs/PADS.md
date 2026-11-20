# Pad instruments — Geonkick MPC0.0.21

Open **MASTER → KIT**. All sixteen sounds initially contain Init. Select01–16
with Touch, then edit using the existing SOUND/LAYER/ENVELOPE/MASTER pages. Their
header shows the selected pad. Selecting does not play it; PLAY SELECTED PAD
previews it. PRESETS and INIT SELECTED PAD affect only this one instrument.

**Single** is the default, preserving the previous chromatic behavior: every
MIDI note plays the selected sound, with optional Key Track. **16 Pads** maps
sixteen consecutive MIDI notes to the sixteen sounds. FIRST MIDI NOTE is the
first assigned note, default36 (36–51); use48 or60 if that matches your MPC track.
Read the MIDI number rather than relying on host octave naming. Notes outside
the assigned range are ignored. In kit mode, notes select sounds without pitch
transposition; Tune remains independent for each sound. Channel is not filtered.
Note input triggers a sound without changing the sound selected for editing.

OUTPUT on the sound pages is per sound. KIT OUTPUT controls the mixed instance.
Poly/Cut/Gate are per sound; Cut retriggers only that sound, and a Gate note-off
still reaches its voice after editing another pad. CC120/123 clear every voice.
Sixteen playback voices are shared; overflow steals the oldest with a short
smooth tail. All sounds feed the existing stereo output.

Save the MPC project or a native MPC plugin preset to store the entire kit.
GKMP8 includes all sound parameters/curves, selection and routing/output settings.
Earlier GKMP1–7 states load into pad01 in Single mode, other pads reset to Init.
Older builds cannot read the new kit state. The original.gkick/.gkit import is
not implemented in this build; it is the following task. Native MPC saving is
used rather than implementing a competing save browser.

Q-Links control rotary knobs only. Selection/routing are Touch. Existing shared
host automation controls address the currently edited sound, not sixteen new
independent parameter sets; dedicated per-pad automation is deferred.

Pad layout matches the MPC:01 bottom-left,04 bottom-right,13 top-left,16 top-right.

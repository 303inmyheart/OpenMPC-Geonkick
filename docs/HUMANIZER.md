# Humanizer — Geonkick MPC0.0.20

Open MASTER → HUMANIZER. The effect starts **OFF**, including after Init/reset
and when loading an older project. Enable it with Touch; only the two knobs
have Q-Links.

- **VELOCITY VARIATION:**0–30%, randomly lower/higher than the incoming MIDI
  velocity. The original integer-velocity calculation is bounded to1–127.
  Hardware steps the amount by0.1%; Touch moves continuously.
- **TIMING VARIATION:**0–30ms, a random delay from zero up to that maximum.
  It never moves a hit earlier. The original randomizer uses0.25ms buckets,
  converted to the nearest audio sample. Hardware steps the maximum by0.25ms;
  Touch moves continuously.

Start with roughly5% /2–5ms on a repeated pattern. Velocity-only works with
Timing0, timing-only with Velocity0. Both amounts0, or OFF, retain the exact
original timing/audio. The base rendered waveform is unchanged: Humanizer
varies individual played hits, not the underlying sound's synthesis.

Each arriving hit retains its sound, gain, tuning and playback mode. Already
playing hits keep their cached sound when parameters change. Cut stops/fades
previous voices at the delayed onset, not when the note arrives. Gate Note-Off
cancels a matching pending note; Poly/Cut keep their percussion-tail behavior.
Panic/all-notes-off and state/reset cancel future queued hits as well as the
appropriate active sound. Preset changes cancel pending hits while retaining
the established behavior of already sounding notes.

Scheduling is sample-based and fixed-size, without audio-thread allocation,
sleep or synthesis waits. Up to64 hits can wait within the maximum30ms window;
pathological overflow retires the earliest queued event. This is a safety bound,
not a new instrument/voice count: playback still has eight voices.

Enable/amounts are saved in GKMP7. Older GKMP1–6 states load with Humanizer OFF;
filter modes from GKMP6 are retained. Random sequence position and transient
pending notes are deliberately not saved. New GKMP7 states require0.0.20+.
The implementation uses the original GPL Geonkick Humanizer/randomizer with the
MPC adapter's own bounded cached-hit scheduler; no new dependency is bundled.

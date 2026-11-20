# Samples — Geonkick MPC0.0.23

1. Select the sound on MASTER → KIT. Open MASTER → SAMPLES and choose the target
   oscillator (L1/1–L3/3). Each of the16 pads can retain samples on all9 oscillators.
2. Press OPEN STORAGE (starts at internal storage). DRIVES opens the filesystem
   root for other mounted storage. Tap folders to enter them; UP returns. The
   list shows folders and.wav files,8 at a time, with PREVIOUS/NEXT page buttons.
3. Tap a WAV to select it, then LOAD / REPLACE OSC. Loading happens in the worker.
   A failed/unsupported file leaves the existing asset unchanged. The target is
   the pad/oscillator selected when Load was pressed. Resetting/replacing that
   target cancels an old pending import rather than overwriting the newer sound.
4. PLAY PAD auditions the mixed selected sound. Import turns the chosen oscillator
   on and changes it to Sample, flat Amp and original-speed pitch. It retains
   other oscillators, layer gains and filter/drive settings. If the status says
   LAYER IS OFF, enable that layer’s gain. Its sample is loaded even while muted.
5. Existing pitch controls now show semitone offsets in Sample mode. One fine
   Q-Link tick remains5cents; Touch stays continuous/coarse. Pitch Drop and the
   point-editable Pitch envelope affect playback speed. Pad Tune/Key Track keep
   their previous meaning. The Sample/Synth switch retains the loaded asset;
   CLEAR SAMPLE frees that asset and turns the chosen oscillator off.
6. Save the MPC project or native plugin preset: audio is embedded. The source
   WAV can subsequently be removed. Folder paths and browser navigation are not
   stored. Saving/loading entire large sample presets is a host state operation,
   intended with transport stopped; interactive file import is worker-driven.

Supported: little-endian RIFF/WAVE PCM8/16/24/32, IEEE float32/64 and extensible
PCM/float formats,8–192kHz,1–8 input channels. The first input channel is used,
matching Geonkick’s mono engine. Linear conversion produces44.1kHz PCM. The first
4seconds are imported; the status reports truncation. Finite out-of-range float
values are clipped to full scale; NaN/Inf, compressed/corrupt files and unsupported
formats are rejected. FLAC/OGG/MP3 import is not included yet. No normalization or
factory/user audio is bundled. A bounded one-shot end fade prevents abrupt source
endings; raw saved sample data itself is retained.

Budget:10MiB of decoded float32 PCM across the instance. Assets remain available
when switching to Synth; clear unused samples to reclaim that budget. Complete
state is limited to16MiB, with base64 assets. The existing1551 physical values
per pad and learned note map remain unchanged. Optional VIEW1 remembers the three
layer targets; SMP1 contains unique pad/oscillator assets, source mode, count,
filename and float PCM. These extensions require0.0.23 or newer.

SOUND tabs each show the remembered oscillator/envelope for their own layer.
Their quick sliders follow the same target. Editing an oscillator selects it
for that layer; switching tabs itself only displays the already cached view.
No new synthesis or per-column host callback is required on tab switching.

Format references: [Microsoft WAVEFORMATEX](https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatex),
[Microsoft WAVEFORMATEXTENSIBLE](https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatextensible).

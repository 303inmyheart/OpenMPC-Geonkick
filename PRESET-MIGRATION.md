# Read before updating: Geonkick MPC 1.0.1

**The manufacturer changes from `OpenMPC` to `303inmyheart`. Older personal MPC presets
can need migration even though the plugin UID stays the same. The installer
does not migrate user preset files automatically.**

## What users may see

The MPC can reject an older `.xpl` with “This state file is not for Geonkick MPC”
before passing its sound data to the synth. Do not overwrite that preset with
Init: the rejection does not establish that its sound data are damaged.

## Recommended: the Python converter

Use the [common preset converter](docs/PRESET-CONVERTER.md) for both JC303 MPC
and Geonkick MPC. It previews the changes, creates separate compatible copies
and leaves your original files and encoded sounds unchanged. It runs on your
computer; the plugin installer does not convert personal presets automatically.
Manual XML editing below is an alternative, not a requirement.

## Keep the original; migrate a copy

1. Before updating, back up your own presets and projects outside the installed
   plugin folder. Keep the original files unchanged.
2. If a preset is rejected, prepare a separate copy of its `.xpl`. In its XML
   `PLUGIN` metadata, change `manufacturer="OpenMPC"` to
   `manufacturer="303inmyheart"` and set `file` to the actual installed `.so` path:
   `<drive>/VST2 Plugins/303inmyheart - VST - Geonkick MPC/geonkick_mpc.so`. Replace `<drive>` with the drive
   containing the installed plugin; do not use that placeholder literally.
3. Preserve `uid="476b4d33"` and the entire encoded `state` element exactly.
   Do not change the state payload, the preset's other sound data or UID.
4. Save the copy in your normal MPC user preset storage under
   `MPC Documents/Plugin Presets/Instruments/303inmyheart - VST - Geonkick MPC/`.
   Use a different filename if one already exists. Never overwrite an existing
   preset. Reopen the native preset list and load the compatible copy.
5. Check the sound before saving further edits. Keep your original backup.

The separate Python converter handles the supported XPL header changes. The
plugin installer only migrates installed plugin folders/settings; it does not
run the converter. Keep originals and check the copied sounds on the MPC.

## Old-project option

If the sound still plays correctly in an older project, use MPC's normal preset
Save command to save it under a **new name**. Then load the newly saved preset in
a fresh Geonkick instance. Keep the old project/preset as backups. This route
avoids file editing; the converter is useful when only the older XPL remains.
Check the sound after either route; native Save-as was not exhaustively tested.

## Validation scope

The current Geonkick engine accepts the historical GKMP9 state used in the
local check. The maintainer confirmed loading an older Geonkick project with
the manufacturer-corrected version and reported that everything matched.
A separately created historical preset copy retains its original encoded state;
exhaustive loading of every converted preset is not claimed.

Existing project reopening across the manufacturer change is a separate check;
no blanket project-compatibility claim is made.

## In-plugin guide

PRESET INFO in the header opens a plain-language English explanation. The
converter is a separate computer tool; it is not run automatically by the plugin.

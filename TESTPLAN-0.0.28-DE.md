# Geonkick MPC0.0.28 testen

1. In Sounds → Plugin Geonkick MPC öffnen. Acht Startpresets inklusive Init
   und „Starter Kit (16 Pads)“ müssen vorhanden sein. Auch das native Preset-Menü prüfen.
2. Deep Kick laden, mehrere Regler ändern, dann Init laden: der vollständige
   Init-Zustand muss wiederhergestellt werden. Weitere Startpresets anspielen.
3. Starter Kit laden. Falls die Hardware-Zuordnung abweicht, bei gestopptem
   Transport unter MASTER/KIT LEARN16 aktivieren und die gewünschte Bank1–16
   einlernen. Alle16 Sounds müssen erreichbar sein.
4. Zwei Pads bearbeiten; INIT PAD darf nur das ausgewählte Pad zurücksetzen.
5. Eigenes Kit über das native MPC-Preset-Menü speichern. Andere Startpresets
   laden, eigenes Kit zurückladen: beide Sounds, Zuordnung und ggf. Samples
   müssen wieder da sein. Auch nach Plugin-/Projekt-Neuladen prüfen.
6. SOUND-Layer-Tasten, ENVELOPE und feine Q-Links kurz gegenprüfen. MASTER hat
   KIT/SAMPLES/MIX/FX; die interne PRESETS-Seite ist entfernt.

Native Presets ersetzen grundsätzlich den gesamten Plugin-Zustand. Die acht
Startpresets sind MPC-Rezepte, nicht die komplette Original-Geonkick-Bibliothek.
Nächster Roadmap-Schritt: Redesign inklusive WAV-Vorhören vor dem Laden.
Original.gkick/.gkit-Import folgt später. Kein finaler Store-Benchmark jetzt.

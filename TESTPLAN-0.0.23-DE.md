# Geonkick MPC0.0.23 — Sample-Import und Sound-Kurven

1. MASTER → KIT: einen Pad-Sound wählen. MASTER → SAMPLES: zunächst L1/1.
   OPEN STORAGE, Ordner öffnen, WAV antippen, LOAD / REPLACE OSC. PLAY PAD und
   Hardware-Pad testen. Die gelernte Pad-Zuordnung muss unverändert funktionieren.
2. Eine andere WAV auf einem zweiten Pad laden: Sounds müssen unabhängig bleiben.
   Optional auf L2/2 oder L3/3 laden; deren Layer-Gain einschalten, falls0.
3. Auf der Layer-Oszillatorseite SYNTH/SAMPLE umschalten. Geladener Name und
   Sample-Pitch müssen passen. Q-Links weiterhin fein, Touch direkt. Pitch Drop
   und vorhandene Amp/Pitch-Punkte ausprobieren. Keine Zusatz-Modulationskurven.
4. Projekt oder natives Plugin-Preset speichern/neu laden. Audiodaten und
   Pad-Zuordnung müssen erhalten bleiben, auch ohne ursprüngliche WAV-Datei.
5. SOUND LAYER1: z.B.Osc2 kurz einstellen; LAYER2: Osc2 langsam einschwingen;
   LAYER3: Osc3 lang. SOUND-Tabs wechseln: Kurve, Zielanzeige und Schnellregler
   müssen jeweils zur richtigen Layer passen. Tab-Wechsel darf nichts neu
   synthetisieren oder laufende Stimmen abschneiden.
6. Sample bei klingender Note ersetzen/löschen: alte Note behält den vorbereiteten
   Klang; neue Noten verwenden den fertig geladenen Sound. Kurze Samples und
   Cut-Retrigger auf neue Knackser prüfen. CLEAR SAMPLE schaltet den Ziel-Oszillator aus.

WAV PCM/Float, erster Kanal, maximal erste4Sekunden,10MiB PCM insgesamt. Fehler-
oder Limitmeldung darf einen vorhandenen Sample-Sound nicht zerstören. Neue
Original-Preset-/Soundbank-Importe folgen später über die native MPC-Verwaltung.

Roadmap: Sample-/Kurventest → native Presets/Soundbanken → kleines KIT-Tutorial
vor Release → vollständige Release-Abnahme und erst dann Store-Benchmark.

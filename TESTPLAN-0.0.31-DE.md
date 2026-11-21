# Geonkick MPC 0.0.31 — Redesign und Sample-Vorhören

Entwicklungsbuild; Offline-Prüfung ist kein Hardwaretest und kein Store-Benchmark.

1. SOUND: LAYER 1/2/3 wechseln. Kurve, angezeigte Regler und Q-Links müssen
   dieselbe Ebene betreffen. Alle drei Oszillatoren einzeln an-/ausschalten und
   Frequency/Level mit Touch und Q-Links prüfen.
2. LAYER 1–3: zuerst OSC 1/2/3, danach FX 1/2/3. Je Seite vier Q-Links anwählen:
   Der Rahmen muss zum richtigen Reglerblock passen, inklusive Namen/Werten.
   Schalter und Auswahlen bleiben per Touch bedienbar und ohne Q-Link-Belegung.
3. ENVELOPE: große COPY SIMPLE-Taste, Amplitude/Pitch, Simple/Points, Punkte
   verschieben. Kurve und Touchposition müssen weiterhin synchron bleiben.
4. MASTER → SAMPLES: Ordner öffnen, WAV antippen. Ohne Load muss es hörbar sein;
   erneutes Antippen desselben WAVs stoppt die Vorschau. PLAY PREVIEW wiederholt,
   STOP beendet sie. Mehrere WAVs schnell antippen und Ordner wechseln.
5. Nach dem Vorhören den bisherigen Pad-Klang spielen: er muss unverändert sein.
   Erst LOAD SAMPLE ersetzt den gewählten Pad/Oszillator. Andere Pads/Layers
   müssen unverändert bleiben. REMOVE FROM OSC betrifft das zugewiesene Sample.
6. SET DEFAULT und OPEN DEFAULT, acht Dateizeilen und BACK/NEXT, lange Dateinamen,
   leere Ordner und ungültige WAVs prüfen. Keine fehlenden/übermalten Dateinamen.
7. Bestehendes Projekt und Factory-Kit laden, speichern und erneut öffnen.

Weiter offen: native Ordner im Edit-Preset-Dropdown (keine unterstützte VST2-
Kategorie-Schnittstelle gefunden); Thumbnail-Wiedereinstieg bei externen Plugins
auf Wunsch zurückgestellt. Allgemeiner .gkick/.gkit-Import bleibt späterer Schritt.

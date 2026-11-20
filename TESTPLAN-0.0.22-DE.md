# Geonkick MPC0.0.22 — Hardware-Pads zuordnen

1. Sequenz stoppen, auf der Live III Bank A wählen. MASTER → KIT öffnen.
2. Bei LEARN HARDWARE PADS auf LEARN16 tippen. Status zeigt PRESS PAD01/16.
3. Hardware-Pads1–16 einmal in Nummernfolge spielen:1 unten links, dann2/3/4,
   darüber5–8, dann9–12 und oben13–16. Anzeige nennt jeweils das nächste Pad.
4. Am Ende steht16 PADS LEARNED und NOTE MAP schaltet auf Learned.
5. Hardware-Pad1 muss Sound1 spielen, Hardware-Pad2 Sound2 usw. Insbesondere
   das vierte Pad der dritten Reihe von oben muss Sound8 statt Sound1 spielen.
6. Unterschiedliche Presets auf1/2/8 laden. Projekt oder natives Plugin-Preset
   speichern/neu laden: Zuordnung und Sounds müssen erhalten bleiben.
7. Optional Lernvorgang starten, zwei Pads spielen, OFF: die bisherige Zuordnung
   muss erhalten bleiben. PLAY SELECTED PAD darf keine Hardware-Lernschritte zählen.

Wenn zweimal dieselbe MIDI-Note kommt, bleibt der Lernschritt stehen. Bei einem
versehentlichen Doppelanschlag das nächste richtige Pad spielen. Wenn zwei
unterschiedliche Pads dieselbe Note senden, zunächst eine eindeutige Notenkarte
in Notes/Pad Perform wählen, etwa Chromatic, und neu lernen. Nach späteren
Tonleiter-/Oktavwechseln die dadurch veränderte Notenbelegung erneut lernen.

Native/ARM64-Tests sind keine Hardware-Abnahme und kein Store-Benchmark.
Danach: Sample-Import pro Pad, originale.gkick-Presets und.gkit-Soundbanken.

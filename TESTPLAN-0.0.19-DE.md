# Geonkick MPC 0.0.19 — Filter prüfen

1. Einen neuen Init-Sound öffnen. MASTER → FX zeigt FILTER MODEL = WIDE.
   FILTER einschalten und CUTOFF langsam/schnell von20 Hz bis20 kHz bewegen.
   Q-Link/Datenrad ist fein, Touch deckt den ganzen Bereich kontinuierlich ab.
2. LOW PASS, HIGH PASS und BAND PASS mit RESONANCE0%, mittleren Werten und100%
   prüfen. Der Klang darf bei hoher Resonanz deutlich lauter/spitzer werden;
   das Ausgangssignal bleibt am vorhandenen Limiter begrenzt. Auf neue Knackser,
   Stille oder Blockaden achten. Q wird im Wertefeld zusätzlich angezeigt.
3. Dasselbe an LAYER1 → FX1 und weiteren Oszillator-FX testen: jeder Filter ist
   unabhängig, seine WIDE/CLASSIC-Wahl ebenso. Nur Drehregler haben Q-Links.
4. CLASSIC wählen: Cutoff bleibt auf30–8000 Hz begrenzt, Resonanz nutzt den alten
   Wertebereich. Touch darf an den Grenzen nicht auf einer falschen Position
   hängen bleiben. Beim Modellwechsel bleiben je Modell eigene Werte erhalten.
5. Ein älteres Projekt laden: alle Filter müssen CLASSIC anzeigen, Klang und
   alte Automationen bleiben erhalten. Neue Projekte als neue Datei speichern;
   neu gespeicherte GKMP6-Zustände benötigen0.0.19 oder neuer.
6. Neue WIDE-Werte speichern/laden. Oszillator- und Masterwerte bleiben getrennt.
   Closed/Open Hat sollen ihren bisherigen Klang behalten und CLASSIC nutzen.
7. Einen langen Hit halten und Filter verändern: der laufende Hit bleibt gleich,
   neue Schläge übernehmen abgeschlossene Änderungen. Schnelle Retrigger und
   Poly/Cut/Gate prüfen. Punkteditor und Tasten müssen wie in0.0.18 funktionieren.

Installiert, MPC-Anwendung neu gestartet und Bibliothekshash geprüft.
Native/ARM64-Funktionstests und Paketprüfung bestanden. Neuer Hör-/Touchtest auf
Live III steht noch aus; kein formaler Store-Benchmark dieses Entwicklungsbuilds.

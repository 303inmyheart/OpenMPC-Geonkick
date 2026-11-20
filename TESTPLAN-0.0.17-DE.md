# Geonkick MPC 0.0.17 — Hüllkurven prüfen

Entwicklungsbuild installiert; MPC-Anwendung neu gestartet und Bibliothekshash
geprüft. Neue Touch-/Hörprüfung und formaler Store-Benchmark stehen noch aus. Die Audio-/Menüprüfungen aus 0.0.14/16 gelten
weiter. Neue Projektstände dieser Version benötigen 0.0.17 oder neuer.

1. ENVELOPE öffnen, L1/1 und AMP, dann POINTS wählen. Die bestehende Kurve muss
   erhalten bleiben, die echte gemischte Wellenform sichtbar sein.
2. Mit POINT > / < POINT einen Punkt wählen; Status POINT n / Anzahl beachten.
   ADD POINT mehrmals bis 16 verwenden. Einfügen darf den Klang nicht hörbar
   ändern. Danach werden keine weiteren Punkte hinzugefügt.
3. GRIP GROUP 2–4, 5–7 usw. wechseln. Beide Endpunkte bleiben verfügbar, dazu
   jeweils bis zu drei innere Griffe. Die gesamte Kurve bleibt angezeigt.
4. Griffe langsam/schnell in X/Y bewegen. Unberührte Punkte dürfen nicht driften;
   kein Punkt darf Nachbarn überholen. POINT LEVEL/POSITION müssen den gewählten
   Punkt anzeigen. Eine reine horizontale Geste sowie die Nachbargrenzen prüfen.
5. SEGMENT CURVE auf negative/positive Werte setzen. Das ausgehende Segment
   muss sichtbar/hörbar gebogen werden. Am Endpunkt ist kein Segment vorhanden.
   ADD auf einem gebogenen Segment darf dessen Form nicht hörbar ändern.
6. Innere Punkte bis auf die beiden Endpunkte löschen. Bei 2, 3 und 4 Punkten
   besonders auf überlagerte, unbenutzte native Griffe am rechten Rand achten.
   Danach wieder Punkte hinzufügen. Endpunkte dürfen nicht gelöscht werden.
7. AMP/PITCH und alle neun Oszillatoren wechseln: unabhängige Kurven, richtiger
   Ziel-Oszillator, kein Rücksprung durch einen späteren Graph-Refresh.
8. SIMPLE/POINTS wechseln: Custom-Kurve bleibt erhalten. COPY SIMPLE ersetzt
   sie. Attack/Decay oder Pitch Drop/Time auf den OSC-Seiten aktiviert SIMPLE.
9. Projekt speichern/neu laden, Kurven/Modus/Länge vergleichen. Zusätzlich ein
   älteres Projekt mit einer fünfpunktigen Custom-Kurve laden und bearbeiten.
10. Während schneller Änderungen wiederholt spielen: keine neuen Knackser oder
    langen Audio-Blockaden. Die Grafik folgt nach dem bestehenden kurzen
    Ruheintervall; Q-Links steuern im Editor nur LENGTH, keine Punktparameter.

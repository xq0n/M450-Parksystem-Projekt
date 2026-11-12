# Vollständiger manueller TK-Durchlauf

> Aktueller Aufbau: **Ein SG90** am Schranken-Board, GP8/Pin 11, übernimmt beide Richtungen. Jeder Pico stellt eine eigene Weboberfläche bereit. Der gemeinsame Servo öffnet jeweils für 5 Sekunden.


Diese Checkliste ist **noch nicht ausgeführt**. Hosttests und Kompilierung bestätigen keine WLAN-Verbindung, Servo-Bewegung oder Bedienbarkeit am echten Aufbau.

## Vorbereitung und Rollen

- [ ] Boardmodelle notiert, beide passenden Firmwares mit echten WLAN-Daten übertragen.
- [ ] Board ohne Servo = Kasse/Web-UI; Board mit Servo = Schranke, einzelner SG90 an GP8, GP9 unbenutzt.
- [ ] Schranken-Pico hat eingelötete Stiftleisten oder direkt angelötete Leitungen. Bei USB oben: Rot an VBUS oben rechts/Pin 40; Braun an GND links Reihe 8/Pin 8; Orange an GP8 links Reihe 11/Pin 11.
- [ ] Auf der Schranken-Seite „Servotest ohne Parkvorgang“ mit Admin-PIN 4500 ausgelöst; der SG90 bewegt sich auf 90° und nach 5 Sekunden auf 0°. Wenn nicht, elektrische Kontakte und Versorgung vor den TK-Fällen prüfen.
- [ ] Leichte Modellarme, keine Personen/Fahrzeuge im Bewegungsbereich, Schließen/Endlagen ohne Anschlag geprüft.
- [ ] Beide Picos und Browser im selben WLAN; beide Webseiten unter ihren eigenen IPs öffnen; Schranken-Seite meldet „Kasse: verbunden“.
- [ ] Serieller Monitor 115200 Baud für RPC-Logs verfügbar; PIN bekannt (Demo 4500).
- [ ] Testmodus aktiv; Standardtarif 30 min / 50 Rp. / Tagesmaximum 1500 Rp. / Kulanz 15 min / Kapazität 10.
- [ ] Grundzustand: beide ausschalten, Kasse zuerst, dann Schranken-Pico starten. Keine alten Datensätze.
- [ ] Testdaten: LU 123456, LU 654321, LU 999999, leere Eingabe, XX!?%&.
- [ ] Datum, Tester, Boardmodelle, Firmware-Commit und Verdrahtung im Testprotokoll festgehalten.

TK S. 4–6: ca. 10 min Vorbereitung, 45–50 min Durchführung, 15 min Auswertung, 15–20 min Präsentation. Kilian bedient Schranken/Fahrerrolle, Noel die Kasse; beide protokollieren. Für TC-20 möglichst eine Person ohne Erklärung einsetzen. Aufgaben können für einen zweiten Durchlauf getauscht werden.

Zwischen **unabhängigen** Fällen beide Picos neu starten; zusammenhängende Fälle 01→04/06→09→13 dürfen denselben Datensatz verwenden. Nach jeder Schrankenbewegung mindestens 3 s warten, bis auch die Abschlussmeldung angekommen ist. Bei langem Netzwerkabgleich erst dessen Ende abwarten. Zeitvorstellungen wirken global auf alle Datensätze; danach Gebührenangebote neu anfordern.

## Testfälle TC-01 bis TC-20

| ID / Quelle | Vorbedingung und konkrete Schritte | Erwartetes Ergebnis |
|---|---|---|
| **TC-01**, TK 9–10 | Leerer Grundzustand. Auf der gemeinsamen Seite Einfahrt `LU 123456` bestätigen. | Gemeinsamer Servo öffnet 5 s und schließt. Genau ein Fahrzeug, Einfahrtszeit seit Start, steigende Dauer, unbezahlt. |
| **TC-02**, TK 10 | Einfahrtfeld leer absenden. Für Serverprüfung Browservalidierung umgehen: siehe Befehle unten. | Verständliche Validierung; keine neue Registrierung, keine Bewegung. |
| **TC-03**, TK 10 | `XX!?%&` als Einfahrt eingeben. | Ungültiges Kennzeichen; beide Schranken geschlossen. |
| **TC-04**, TK 10–11 | TC-01 ausgeführt und Schranke wieder zu. Dasselbe Kennzeichen nochmals eingeben, auch als `lu123456`. | Duplikatmeldung, kein zweiter Datensatz/Zeitenwechsel, keine zweite Öffnung. |
| **TC-05**, TK 11 | Neu starten. Kassen-Einstellungen: Kapazität 2, übrige Werte belassen, PIN eingeben. LU 123456 und LU 654321 jeweils einfahren; dann LU 999999 versuchen. | Belegung 2/2; dritter Eintrag mit „Besetzt“ abgelehnt, Einfahrt bleibt zu. Nach bezahlter Ausfahrt eines Fahrzeugs ist wieder ein Platz frei. |
| **TC-06**, TK 11 | Neu starten, LU 123456 einfahren. 1–2 echte Minuten warten oder Testzeit +1 Minute. Gebühr berechnen. | Parkdauer plausibel; CHF 0.50. Bei echter Wartezeit Fortschritt messen. Grenzwerte exakt 30:00/30:00.001 werden im Hosttest geprüft. |
| **TC-07**, TK 11–12 | Neu starten/einfahren. Testzeit +1200 min (20 h), dann neu berechnen. Optional weitere +240 min und danach +1 min. | Bei 20 h CHF 15.00 (Tagesmaximum). Nach etwas mehr als 24 h wegen zusätzlicher realer Sekunden CHF 15.50. Exakt 24 h wäre CHF 15.00; exakte Grenze im Hosttest. |
| **TC-08**, TK 12 | Neu starten/einfahren, Dauer <30 min. Gebühr ansehen. Kasse: Preis von 50 auf 100 Rp. ändern, PIN. Neu berechnen. | CHF 1.00; kein Firmwarebuild nötig. Altes Zahlungsangebot wird abgelehnt. Danach Preis auf 50 zurücksetzen oder neu starten. |
| **TC-09**, TK 12 | Eingefahrenes Fahrzeug, gültiges Angebot. Karte auswählen, Zahlung ausdrücklich bestätigen. | Bestätigung mit Kennzeichen, Betrag, Zahlungszeit und Kulanzende. Datensatz bezahlt, Betrag CHF 0.50 bei <30 min. |
| **TC-10**, TK 12 | LU 999999 ist nicht eingefahren. Gebühr dafür anfordern; optional direkten `/pay`-POST unten senden. | „Kennzeichen nicht gefunden“, keine Gebühr/Zahlung/Neuregistrierung. |
| **TC-11**, TK 13 | Neues unbezahltes Fahrzeug, Gebühr berechnen und „Zahlung abbrechen“. Auf Pico 2 Ausfahrt versuchen. | Abbruch bestätigt; unbezahlt, Ausfahrt bleibt zu. Browser-Zurück und altes Zahlungsformular erneut senden: Angebot ungültig. |
| **TC-12**, TK 13 | Zwei Fahrzeuge einfahren. Eines mit Karte, das andere mit TWINT bezahlen. Erneut deren Gebührenansicht öffnen. | Beide simuliert bezahlt, richtige Methode/Zeiten getrennt sichtbar. Keine reale Abbuchung. |
| **TC-13**, TK 13 | Fahrzeug bezahlt, Kulanz noch gültig. Auf der gemeinsamen Seite Ausfahrt auslösen. | Gemeinsamer Servo öffnet 5 s und schließt. Kasse zeigt nach bestätigtem Depart keinen aktiven Datensatz mehr. Gleiches Kennzeichen kann neu einfahren und ist dann wieder unbezahlt. |
| **TC-14**, TK 13–14, **kritisch** | Neues Fahrzeug einfahren, nicht bezahlen, Ausfahrt anfordern. Auch unbekanntes Kennzeichen probieren. | Keine Ausfahrtbewegung. Bei registriertem Fahrzeug „Unbezahlt“, bei unbekanntem „nicht gefunden“. Jede falsche Öffnung = kritischer Fehler, Abnahme stoppen. |
| **TC-15**, TK 14 | Neues Fahrzeug bezahlen, innerhalb von 15 min ausfahren. Bei Bedarf nach Zahlung Testzeit +14 min (zügig weiterarbeiten). | Öffnung ohne weitere Zahlung. Frist exakt einschließlich Ende im Hosttest geprüft. |
| **TC-16**, TK 14 | Neues Fahrzeug bei <1 min für CHF 0.50 bezahlen. Testzeit **+31 min**, Ausfahrt versuchen. An Kasse neu berechnen, Nachzahlung bestätigen, erneut ausfahren. | Zunächst „Kulanzzeit abgelaufen“ und geschlossen; bei Gesamtdauer 31–60 min Nachzahlung CHF 0.50. Nach Bestätigung neue 15-min-Frist und Ausfahrt möglich. Zusatztest +16 min bei Gesamtdauer <30 min: Rest 0, aber Bestätigung trotzdem erforderlich. |
| **TC-17**, TK 14 | Bezahltes Fahrzeug ausfahren. Beide seriellen Logs beobachten. | Pico 2 und Pico 1 zeigen dieselbe `seq`, `op=4`, Kennzeichen, `result=0`, Besuchs-ID; anschließend `op=5` für Abschluss. Öffnen erst nach Autorisierungsantwort, normalerweise deutlich unter 1800 ms. |
| **TC-18**, TK 14–15, **kritisch** | Fahrzeug einfahren/bezahlen, alle Schranken geschlossen. Nur Kasse vom WLAN trennen (z. B. AP-Gerätesperre) oder ausschalten. Ausfahrt anfordern. Verbindung wiederherstellen. | Keine Öffnung. Entweder sofort „nicht verbunden“ oder nach spätestens 1800 ms Protokollfrist Störung (Webaktualisierung bis ca. 2 s zusätzlich). Späte Freigabe führt nie zu späterer Bewegung. Bei WLAN-Rückkehr ohne Neustart wird abgeglichen/storniert; danach neu anfordern. Bei ausgeschalteter Kasse Datenverlust melden/erkennen, neuen Test beginnen. |
| **TC-19**, TK 15 | LU 123456 und LU 654321 nacheinander einfahren, getrennt bezahlen und ausfahren. Dazwischen Status jeweils ansehen. | Unabhängige Zeiten/Beträge/Methoden; nach erster Ausfahrt bleibt nur das zweite Fahrzeug, nach zweiter keines. |
| **TC-20**, TK 15 | Unvorbereitete Person erhält nur die beiden geöffneten Startseiten. Soll einfahren, bezahlen, ausfahren und einen Fehler korrigieren. Keine zusätzliche Erklärung. | Reihenfolge, Felder, Auswahl, ausdrückliche Bestätigung, Abbruch, Erfolg und Störung werden verstanden. Probleme wörtlich protokollieren; nicht automatisch als bestanden markieren. |

TC-05, TC-15 und TC-16 sind implementiert und daher bei diesem Stand **anzuwenden**, nicht als „n. a.“ zu überspringen.

### Negative HTTP-Eingaben ohne Browservalidierung

IP-Adressen zuerst auf eigene Werte setzen. Diese Anfragen ändern den Zustand nur, wenn sie gültig und fachlich zulässig wären:

```powershell
$kasse = 'http://192.168.1.40'
$schranken = 'http://192.168.1.41'
curl.exe -i -X POST "$schranken/entry" --data 'plate='
curl.exe -i -X POST "$schranken/entry" --data-urlencode 'plate=XX!?%&'
curl.exe -i -X POST "$kasse/pay" --data 'plate=LU+999999&quote=1&method=card'
```

Werte sind Beispiele; die Firmware gibt die tatsächlichen IPs aus. Alle drei Requests müssen eine Fehlerseite liefern, ohne Schrankenöffnung. Automatisierte Hosttests prüfen zusätzlich fehlerhafte Content-Length, überlange/abgebrochene Requests und kaputte Formularkodierung.

## Zusätzliche Hardware-/Robustheitschecks

- [ ] **Start:** Ohne USB-Monitor autonom gestartet; beide Servos unmittelbar in Schließposition. Keine Öffnung durch Boot/Discovery/Heartbeat.
- [ ] **Strom:** Beide Servoanschlüsse stimmen; kein Brownout/Neustart bei Bewegung; gemeinsame Masse geprüft; kein GPIO mit 5 V verbunden.
- [ ] **Winkel:** Öffnungs-/Schließlage sicher erreicht, kein Anschlag; Pulsgrenzen/Montage gegebenenfalls angepasst.
- [ ] **WLAN-Rückkehr:** AP kurz aus/an; beide Webseiten und das Protokoll kehren zurück, ohne manuell zu resetten; Gate bleibt geschlossen bei abgewiesener Aktion.
- [ ] **Bereits offenes Gate:** Nach rechtmäßiger Freigabe WLAN kurz trennen; Gate schließt nach normaler Haltezeit weiter. Abschluss wird nach Rückkehr quittiert.
- [ ] **Langsamer Browser:** HTTP-Verbindung nur teilweise senden/offen lassen; bereits geöffneter Servo muss trotzdem nach 5 s schließen. Reale Abweichung messen und protokollieren.
- [ ] **Doppelklick/Reload:** Keine zweite Zahlung, Fristverlängerung oder neue Öffnung aus alter Antwort. Neue Kennzeicheneinfahrt hat neue Besuchs-ID.
- [ ] **Reboot Kasse:** Keine Freigabe mit altem Bezahlstatus; leeres Register und neue Sitzung erkennbar.
- [ ] **Reboot Schranken während Ausfahrt:** Keine automatische erneute Öffnung; unklare Passage wird geprüft, beide neu starten für neuen Test.
- [ ] **Ungültiger Admin-PIN:** Keine Tarif-/Kapazitäts-/Testzeitänderung.
- [ ] **Testmodus aus:** Mit `PARKING_TEST_MODE=false` neu bauen; `/advance` abgelehnt, normale Gebühren laufen mit echter Zeit.
- [ ] **Speicher/Last des Modells:** Bis zu 32 aktive Fahrzeuge über konfigurierte Kapazität verwaltbar; 33. Einfahrt abgelehnt. Keine Behauptung eines Lasttests über TK-Umfang hinaus.

Gezielten UDP-Paketverlust, beliebige kaputte Frames und CRC-Bitfehler erzeugt der deterministische Hosttest, ohne die physische Hardware zu bewegen. Die echte UDP-/WiFi-Anbindung ist dennoch mit TC-17/18 am Netz zu prüfen.

## Protokoll, Fehler und Ende

Jede Zeile mit Datum, Tester, Ist-Ergebnis, bestanden/fehlgeschlagen/n. a., Schweregrad und Bemerkung in `TESTPROTOKOLL.md` ausfüllen. Unausgeführte Fälle bleiben **offen**. Fehlerklassifikation gemäß TK S. 7–8:

- **Kritisch:** Unbezahlte/unberechtigte Öffnung oder finanzielle Auswirkung. Aufbau nicht abnehmen; Fehler beheben und erneut prüfen.
- **Hoch:** Kernfunktion falsch, beispielsweise Tarifberechnung. Ebenfalls vor Abnahme beheben.
- **Mittel:** Soll-/Teilfunktion falsch, beispielsweise Kulanz.
- **Niedrig:** Darstellung/Verständlichkeit ohne Kernfunktionsfehler.

Nach jeder Korrektur betroffenen Fall und angrenzenden Ablauf erneut testen. Abschluss erst nach protokollierten TC-01–20, keiner offenen kritischen/hohen Abweichung und ausgefüllter Zusammenfassung. Fachliche Abnahme durch Lehrperson/Klasse, nicht durch den Hosttest.

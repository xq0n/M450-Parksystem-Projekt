# Anforderungen, Quellen und Entscheidungen

## Ergänzung: Testdoubles-Auftrag A02 (20.11.2026)

Zusätzlich geprüft wurden `A02-01-Testdoubles-Parksystem.pdf` (3 Seiten) und `T03-02-Testdoubles.pdf` (4 Seiten). A02 definiert den Arbeitsauftrag, T03 erläutert Dependency Injection sowie Dummy, Fake, Stub, Mock und Spy. Daraus folgt keine Pflicht, alle fünf Arten oder ein bestimmtes Mocking-Framework zu verwenden.

| A02-Anforderung | Umsetzung / Nachweis |
|---|---|
| Abhängigkeiten, benötigtes Verhalten, Probleme im Test und begründete Double-Wahl dokumentieren (S. 1–2) | README-Tabelle zu Schranke, Zeit, Transport, entfernter Kasse, Kennzeicheneingabe und Fahrzeugspeicher |
| Logik ohne direkte Hardwareaufrufe, Schnittstellen von außen übergeben (S. 3) | `Barrier`, `TickSource`, `CashLink`, `PlateInput` und `VehicleStore`; produktive Adapter werden in der Firmware zusammengesetzt |
| Tests ohne Hardware (S. 1) | Ausführbare C++-Hosttests mit Schranken-/Speicher-Mock, Fake-Uhr, Transport-Fake, Kassen-/Eingabe-Stub; zusätzliche Integration mit echtem RAM-Speicher |
| README mit Mitgliedern, Double-Analyse, Testkonzeptbezug und Fazit (S. 3) | Alle vier Punkte in der README enthalten; mechanische Prüfung, reales WLAN und Usability werden als manuell verbleibend begründet |
| Branch `testdoubles` und Git-Abgabe (S. 3) | Bestehender Arbeitsbranch `testdoubles`; Veröffentlichung des aktuellen Abgabestands per Commit und Push |
| Anwendung funktioniert weiterhin auf den Picos (S. 3) | Gebühren-/Zahlungs-/Controllerlogik und Hardwaretreiber beibehalten; vier echte Firmware-Builds erfolgreich. Neuer Funktionstest am Aufbau bleibt offen, weil keine Boards angeschlossen erkannt wurden |

Die aktuelle Verifikation mit 17 Testgruppen, 11 741 Prüfungen und den vier Firmware-Builds steht in [TESTERGEBNISSE.md](TESTERGEBNISSE.md). Die folgenden Abschnitte dokumentieren die älteren fachlichen Quellen und Entscheidungen.

> Aktualisierung zum tatsächlichen Aufbau: Der Nutzer hat **eine gemeinsame Schranke** bestätigt. Ein SG90 an Pico 2, GP8/Pin 11, übernimmt beide Richtungen; GP9 entfällt. Angaben zu zwei Servos im ursprünglichen Entwurfs-/Teststand unten sind damit überholt. Aktuelle Verkabelung siehe README.


Stand: 12.11.2026. Beide PDFs wurden vollständig als Text und als gerenderte Seiten gelesen, einschließlich Tabellen, Pinout und Servoabbildungen. Im Repository lagen keine PDFs; verwendet wurden die vom Auftraggeber genannten Dateien in `C:/Users/NMerz/Downloads/`.

| Kürzel | Quelle | Umfang |
|---|---|---|
| A | `A01-03-umsetzung-parksystem (1).pdf`, unveränderte [Repositorykopie](sources/umsetzung-parksystem.pdf) | 3 PDF-Seiten |
| TK | `tk.pdf`, „Testkonzept: Parksystem mit automatischer Schrankensteuerung“, unveränderte [Repositorykopie](sources/testkonzept.pdf) | 18 PDF-Seiten |

Seitenangaben bezeichnen die PDF-Seite ab 1. Die Unterlagen sind fachliche Quellen. Die aktuelle Nutzeranweisung bestimmt die Arbeit am Repository, insbesondere zusätzliche automatisierte Tests.

Unveränderte Quelldateien, SHA-256: A `4c6710aa0a4d44fd4fb0b25aa30b3c7c87f1633968326fb6e4f7f92a37ff318e`; TK `affdc792e62a2adb6078fef34c8ad4591ef9153d9520b71e07524fe743ccb0af`.

## Vollständiger Quellenabgleich

| Quelle | Inhalt und Konsequenz |
|---|---|
| A S. 1 | Genau zwei Picos, Arduino; mindestens Einfahrt, Ausfahrt, Gebührenberechnung, Bezahlung. Git-Repository mit Quellcode und README einschließlich Gruppenmitgliedern, Umsetzungsbeschreibung, User Stories. Unterrichtsdauer 180 Minuten, Gruppe 1–2 Personen; dies ist der Unterrichtsrahmen, keine Laufzeitvorgabe an die Firmware. |
| A S. 2–3 | Pico-Pinout, SG90 mit Signal/VCC/GND. Beispiel GP8 (physischer Pin 11), GND (Pin 8), VBUS 5 V (Pin 40). Kein verbindlicher zweiter Servopin und keine Sensorbeschaltung. |
| TK S. 1 | Ziel: Nachweis aller 14 US; das referenzierte Anforderungsdokument liegt allerdings nicht vor. |
| TK S. 2 | Pico 1 stellt die Kassen-Webseite für Gebühren und Zahlung bereit. Pico 2 stellt die Schranken-Webseite für manuelle Kennzeicheneingabe bei Ein- und Ausfahrt bereit und fragt bei der Ausfahrt den Bezahlstatus bei Pico 1 ab. Der Nutzer bestätigte einen physischen Servo auf dem Schranken-Board. |
| TK S. 3 | Automatisierte Tests, Last-/Sicherheitstests, physische Tests über den Prototyp hinaus sowie US-10–14 sind aus dem TK-Umfang ausgeschlossen. Systemtests über Weboberflächen, Funktion-/Negativ-/Schnittstellen-/Regressions-/Usabilitytests. |
| TK S. 4 | 2× Pico (W), gleiches WLAN, zwei Webseiten über die individuellen IPs, Neustart/Reset zwischen unabhängigen Fällen. Vorbereitung ca. 10 min, Tests 45–50 min, Nachbereitung 15 min, Präsentation 15–20 min. |
| TK S. 5–6 | Manuelle Tests in Reihenfolge, Vergleich Soll/Ist, protokollieren und nach Fehlerkorrektur wiederholen. Kilian: Fahrer/Schranken; Noel: Kasse; beide Testverantwortliche. Lehrperson/Klasse beurteilt fachlich. |
| TK S. 6–8 | Eintritt: beide Weboberflächen erreichbar, Testdaten bereit. Ende: alle Fälle protokolliert, Fehler klassifiziert, Bericht vorhanden. Kritisch: unbezahlte Öffnung; hoch: Kernfunktion/Gebühr; mittel: Soll-/Teilfunktion; niedrig: Darstellung. Kritische/hohe Fehler verhindern Abnahme (S. 8); strengere Regel gegenüber der allgemeineren Endebeschreibung S. 7. |
| TK S. 7 | Testdaten LU 123456, LU 654321, leer, Duplikat, XX!?%&; TC-10 zusätzlich LU 999999 (S. 12). |
| TK S. 8–15 | TC-01 bis TC-20; Einzelzuordnung siehe README-Anforderungsmatrix und Hardwarecheckliste. |
| TK S. 16–17 | Protokollfelder: TC, Datum, Tester, Ergebnis, Schweregrad, Bemerkung. Vorlage in `docs/TESTPROTOKOLL.md`. |
| TK S. 17–18 | Risiken: simulierte Kamera, WLAN-Abhängigkeit, eventuell fehlende Features, kleine Testgruppe. Übergabe muss ohne Rückfragen durchführbar sein. |

## Widersprüche, fehlende Informationen und konkrete Entscheidungen

Bewertungsprioritäten aus TK S. 8–9: **Muss** TC-01/02/04/06/07/09/10/11/13/14/17/18; **Soll** TC-03/08/12/15/16/19/20 (15/16 im alten Prototyp bedingt); **Kann** TC-05 (bedingt). In diesem Projekt sind alle 20 anwendbar. Manuelle Ergebnisse müssen protokolliert werden; kritische/hohe Abweichungen verhindern die Abnahme.

1. **14 US vs. tatsächlich beschriebener Umfang:** TK S. 1 verspricht jede US-01–14 zu prüfen; S. 3 nimmt US-10–14 aus. US-03 und US-09 werden nicht inhaltlich beschrieben. A S. 1 verweist ebenfalls auf ein fehlendes Anforderungsdokument. Implementiert werden sämtliche konkreten TC-01–20, einschließlich optionaler TC-05/15/16. Vollständige Erfüllung der unbekannten 14 US wird nicht behauptet. Für US-10 Dashboard, US-11 Tagesumsatz, US-12 Dauerparkkarten, US-13 Notfallöffnung und US-14 Datenlöschung fehlen Akzeptanzkriterien; sie bleiben außerhalb des definierten Prototyps. Die aktive Fahrzeugliste ist keine Behauptung einer vollständigen US-10-Umsetzung; RAM-Reset ist keine vollständige US-14.
2. **Automatisierte Tests:** TK S. 3/5 beschränkt sich auf manuelle Tests. Die aktuelle Nutzeranweisung verlangt zusätzlich automatisierte Tests. Diese ergänzen die manuellen Systemtests; sie ersetzen keine Hardwareabnahme.
3. **Modell unbekannt:** Der Nutzer bestätigt WLAN, aber nicht das Modell und keine angeschlossenen Boards. Das Projekt unterstützt Pico W (RP2040) und Pico 2 W (RP2350) als getrennte Buildziele. Beide tragen je eine der Rollen. Ein Pico ohne W erfüllt diese WLAN-Architektur ohne Zusatzhardware nicht.
4. **Pinbelegung und Ablauf:** GP8/Pin 11 folgt A S. 3. Der Nutzer bestätigte einen Servo und wünschte direkte Bewegung ohne Sensoren: gemeinsame Schranke öffnet nach erfolgreicher Ein-/Ausfahrt-Freigabe am selben GP8-Ausgang für 5 s. GP9 entfällt. Endlagen und Versorgung sind am Aufbau zu prüfen.
5. **Kommunikation:** TK S. 4/14/15 setzt dasselbe WLAN und eine bei Netzwerkunterbruch gesperrte Ausfahrt voraus. Deshalb WLAN/UDP zwischen den Picos; kein zusätzlicher UART-Draht, der den Netzwerkfehler aus TC-18 umgehen würde. UDP allein ist unzuverlässig; das implementierte Anwendungsprotokoll ergänzt CRC, Sitzungen, Sequenzen, Bestätigungen, Wiederholung, Heartbeat und Fehlerabgleich. Details in `ARCHITEKTUR.md`.
6. **Tarif:** Kein konkreter Betrag, keine präzise Mehrtagesformel. Übernommene, ausdrücklich änderbare Demo-Werte: 50 Rp. je angefangene 30 min, maximal 1500 Rp. je seit Einfahrt begonnenem 24-h-Abschnitt. Volle Tage kosten ebenfalls höchstens den regulären Tagespreis. Kein Kalendertagmodell. Mindestgebühr bei 0 ms = eine Einheit. Änderungen wirken auf aktuelle Neuberechnungen.
7. **Kulanz und Nachzahlung:** TK S. 14 nennt 15 Minuten nur als Beispiel, ohne Nachzahlungsformel. Standard 15 min; exakt bis einschließlich Fristende gültig. Danach ist die Ausfahrt gesperrt. Nachzahlung = neu berechnete Gesamtgebühr minus bereits bezahlte Beträge, mindestens 0. Bereits bezahlte Intervalle und Tagesmaximum werden berücksichtigt. Ist der Rest 0, bleibt trotzdem eine erneute Bestätigung an der Kasse erforderlich. TC-16 mit 31 zusätzlichen Minuten demonstriert eine positive Nachzahlung. Kein doppeltes Abbuchen und keine automatische Fristverlängerung durch erneutes Senden eines alten Zahlungsformulars.
8. **Zeiten:** TK verlangt Zeitstempel, aber keine Kalenderuhr/NTP/RTC. Alle Fachzeiten stammen von Pico 1 als 64-Bit-Laufzeit (einschließlich markierter Testzeit). Keine Synchronisierung zweier Gebührenuhren nötig. Millis-Überlauf wird erweitert; Servo- und Netzwerkfristen verwenden reale 32-Bit-Differenzen. Angebot 120 s, RPC 1800 ms, Retry 300 ms, spätere Abgleichversuche 1000 ms, Heartbeat im Leerlauf etwa 1 s, HTTP 3 s je Phase, WLAN-Neuversuch 15 s sind dokumentierte Implementierungswerte.
9. **Kapazität:** Keine Parkplatzanzahl vorgegeben. Standard 10, über Kassenoberfläche 1–32 konfigurierbar. 32 ist das feste RAM-Limit. Volle Kapazität verhindert Einfahrt mit „Besetzt“.
10. **Persistenz und Durchfahrterkennung:** TK S. 4 erlaubt Neustart zum Löschen. Daten/Tarif sind bewusst im RAM. Stromausfall ist kein persistenter Geschäftsbetrieb. Keine Lichtschranke/Endlagenschalter angegeben; die manuell bestätigte Einfahrt bzw. Ausfahrt und abgeschlossene Haltezeit gelten als Durchfahrt. Nach Controllerneustart während einer Ausfahrt muss der Aufbau geprüft und kontrolliert zurückgesetzt werden. Eine reale Fahrzeugdurchfahrt ist ohne Sensor nicht nachweisbar.
11. **Sicherheitsscope:** Fail-safe bedeutet hier: keine neue Freigabe ohne gültige Antwort. Eine schon rechtmäßig geöffnete Modellschranke beendet ihre normale Haltezeit auch bei WLAN-Ausfall. CRC schützt gegen Übertragungsfehler, nicht gegen Angreifer. Das TK sieht ein isoliertes Übungsnetz und keine Penetrationstests vor; keine Nutzung als produktives Zugangskontroll- oder Bezahlsystem.

## Analyse des übernommenen Codes

Ausgangsstand: Git-Commit `a19b4dd` (first commit), anfangs sauberer Arbeitsbaum. Vollständig geprüft: beide `main.cpp`, beide Konfigurationsdateien, `parking_common.h`, `platformio.ini`, README, Gitignore, VS-Code-Empfehlungen und die drei PlatformIO-Verzeichnisvorlagen. `.pio` und `.pio-core` enthalten erzeugte Dateien/Toolchain, keine zusätzliche Anwendungslogik.

| Bestand | Befund | Umsetzung |
|---|---|---|
| Arduino-Pico/PlatformIO, je eine Firmware | Passend zu A S. 1 und WLAN | Beibehalten; Platform-Commit fixiert; RP2350-Ziele ergänzt |
| Kennzeichen-Normalisierung | Nutzbare Regel, an Arduino String gekoppelt | ASCII-Regel beibehalten, begrenzte Eingaben, hosttestbar |
| Gebühren/Zahlstatus im Pico-1-main | Grundfunktion vorhanden; ganzer Tag immer Maximum; 32-Bit-Zeiten; keine Kulanz | Eigenes Fachmodell, korrekte Kappung, 64-Bit-Zeit, Quote/Bestätigung, Kulanz |
| Karte/TWINT simuliert | TK-konform | Beibehalten, expliziter Abbruch und einmaliges Angebot ergänzt |
| Nur Speicherlimit | Keine fachliche Kapazitätsprüfung | Konfigurierbare 1–32 Plätze |
| HTTPClient auf Pico 2 | Blockierende Requests/DNS, keine Vorgangs- oder Sitzungsnummern | Asynchroner UDP-Transaktionsautomat; kein wartender HTTPClient |
| `/api/departed` nur mit Kennzeichen | Verspätete Meldung könnte falschen/neuen Parkvorgang löschen | Exakte Besuchsnummer plus Sequenz/Sitzung, Antwortcache |
| Servos GP8/GP9 | Echte PIO-Servo-Ansteuerung bereits vorhanden | Beibehalten; Pins/Winkel/Pulse zentral konfiguriert |
| WebServer | Langsame Clients können Schleife aufhalten | Begrenzter HTTP-Parser, begrenzte Arbeit pro loop, Request-/Sendefrist |
| WLAN/mDNS | Verbindungsbeginn blockierend; Wiederanmeldung nur teilweise behandelt | beginNoBlock, Wiederverbindung, UDP-Discovery/IP-Option, Neustart von Sockets |
| Tests | Nur PlatformIO-Vorlagentext | Ausführbare C++-Tests mit echten Fach-/Protokoll-/Controllerklassen |
| README | Teilumfang, keine vollständige Matrix/Checkliste | Vollständiger Aufbau, Betrieb, Tests, Grenzen und Quellen |

Die beiden Rollen, Programmiersprache, Toolchain, Bedienablauf, Demo-Tarif, Kennzeichenformat und Servoanschlüsse bleiben erhalten. Fachlogik und Kommunikation wurden wegen der beschriebenen Fehler und Kopplung neu strukturiert.

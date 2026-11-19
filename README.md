# Parksystem

## Gruppenmitglieder

Kilian Schoeberl, Noel Merz

## Kurze Beschreibung der Umsetzung

Das Parksystem besteht aus zwei WLAN-fähigen Raspberry Pi Picos (Pico W oder Pico 2 W) und ist mit Arduino/C++ umgesetzt. Ein Pico übernimmt die Ein- und Ausfahrt samt gemeinsamer Schranke, der andere die Kasse mit Gebührenberechnung und simuliertem Bezahlvorgang. Die Geräte kommunizieren über WLAN; die Bedienung erfolgt über ihre Webseiten. Ein Servo bewegt die Schranke.

## Umgesetzte User Stories und Anforderungen

Die konkret beschriebenen Anforderungen aus dem Testkonzept (TC-01 bis TC-20) sind umgesetzt:

| Testfälle | Umgesetzte Anforderungen |
|---|---|
| TC-01–05 | Einfahrt, Kennzeichenprüfung, Duplikaterkennung und begrenzte Parkplatzkapazität |
| TC-06–08 | Parkgebühr berechnen, Tagesmaximum und Tarif einstellen |
| TC-09–12 | Zahlung mit Karte oder TWINT simulieren, Zahlung abbrechen und unbekannte Kennzeichen behandeln |
| TC-13–16 | Bezahlte Ausfahrt freigeben, unbezahlte sperren, Kulanz und Nachzahlung |
| TC-17–19 | Status zwischen den Picos abfragen, Kommunikationsfehler behandeln und mehrere Fahrzeuge verwalten |
| TC-20 | Bedienung über die Webseiten |

<details>
<summary>Alle Testfälle TC-01 bis TC-20 anzeigen</summary>

- **TC-01:** Einfahrt mit Zeitstempel
- **TC-02:** Leere Eingabe ablehnen
- **TC-03:** Sonderzeichen ablehnen
- **TC-04:** Doppelte Kennzeichen ablehnen
- **TC-05:** Einfahrt bei vollem Parkhaus ablehnen
- **TC-06:** Mindestgebühr bei kurzer Parkdauer berechnen
- **TC-07:** Tagesmaximum bei langer Parkdauer anwenden
- **TC-08:** Tarif ohne Codeänderung anpassen
- **TC-09:** Zahlung mit Zeitpunkt erfassen
- **TC-10:** Unbekanntes Kennzeichen bei der Zahlung behandeln
- **TC-11:** Zahlung abbrechen
- **TC-12:** Karte oder TWINT auswählen
- **TC-13:** Bezahlte Ausfahrt freigeben
- **TC-14:** Unbezahlte Ausfahrt sperren
- **TC-15:** Ausfahrt innerhalb der Kulanzzeit ermöglichen
- **TC-16:** Nach Ablauf der Kulanzzeit Nachzahlung verlangen
- **TC-17:** Bezahlstatus von Pico 2 bei Pico 1 abfragen
- **TC-18:** Ausfahrt bei Kommunikationsfehler sperren
- **TC-19:** Zwei unabhängige Fahrzeuge verwalten
- **TC-20:** Verständliche Weboberflächen bereitstellen

</details>

Die zugehörigen Hosttests sind vorhanden; die manuellen Hardwaretests müssen gemäß [Hardwarecheckliste](docs/HARDWARETESTS.md) durchgeführt und im [Testprotokoll](docs/TESTPROTOKOLL.md) festgehalten werden.

Die [detaillierten Testfälle mit Prüfschritten und Soll-Ergebnissen](docs/TESTFAELLE.md) orientieren sich am Testkonzept.

## Automatisierte Tests mit Testdoubles

| Abhängigkeit | Rolle und Aufwand im Test | Testansatz |
|---|---|---|
| Gemeinsame Schranke | Öffnet und schliesst per Servo. Der physische Aufbau kann auf dem PC keine Bewegung ausführen. | `BarrierOutput` erhält in der Firmware den Servo-Treiber und im Hosttest einen Mock, der Öffnungs- und Schliessbefehle aufzeichnet. |
| Zeit | Steuert Parkgebühr, Kulanz, Zeitüberschreitungen und Schrankenlaufzeit. Echtzeit würde die Tests verlangsamen. | Die Schrankenstation erhält eine `TickSource`. Auf dem Pico liest `ArduinoTicks` die Laufzeit. Im Hosttest lässt sich die `FakeClock` gezielt vorstellen. Gebühren- und Kulanztests geben Einfahrts-, Zahlungs- und Ausfahrtszeit direkt vor. |
| UDP-Verbindung zur Kasse | Übermittelt Anfragen und Antworten. Paketverluste lassen sich mit realem WLAN schwer reproduzieren. | Die Station erhält einen `CashLink`. Der Transport-Fake `MemoryLink` sammelt ausgehende Nachrichten. Der Testaufbau verarbeitet sie mit dem echten Kassendienst und kann Anfragen oder Antworten verwerfen. |
| HTTP und Kennzeicheneingabe | Liefert Aktionen, Kennzeichen und Formulardaten. Ein Browser erhöht den Aufwand für jeden Testlauf. | Der HTTP-Handler übergibt Operation und Kennzeichen an `GateStation::action`. Der Test ruft dieselbe Schnittstelle mit festen Eingaben auf. Parser-Tests verwenden vorbereitete HTTP-Anfragen. |
| Fahrzeugspeicher | Hält Einfahrtszeit, Zahlung und Belegung. Der vorhandene Speicher arbeitet im RAM und ist pro Test neu initialisierbar. | Die Hosttests prüfen den echten `Store` auf Duplikate, Kapazität und getrennte Fahrzeugdaten. |

Der Schranken-Mock prüft für **TC-13**, dass eine bezahlte Ausfahrt genau einen Öffnungsbefehl und danach genau einen Schliessbefehl auslöst. Für **TC-04, TC-05 und TC-14** prüft er, dass abgelehnte Aktionen keinen Schrankenbefehl auslösen. Die physische Servobewegung und die Verständlichkeit der Weboberflächen (**TC-20**) werden am Aufbau geprüft.

| Testkonzept | Automatisierte Prüfung auf dem PC |
|---|---|
| TC-01 bis TC-05, TC-19 | Kennzeichen, Einfahrtszeit, Duplikate, Kapazität und unabhängige Fahrzeugdaten |
| TC-06 bis TC-08 | Tarifgrenzen, Tagesmaximum und Tarifänderungen |
| TC-09 bis TC-12 | Zahlung, unbekanntes Fahrzeug, Abbruch und Zahlungsmethode |
| TC-13 bis TC-16 | Ausfahrt mit und ohne Freigabe, Kulanzgrenze und Nachzahlung. Für TC-13/14 liefert ein Kassen-Stub vorgegebene Zahlungsantworten. Fake-Uhr und Schranken-Mock prüfen die Schliessgrenze bei 4999 und 5000 ms. |
| TC-17 und TC-18 | Statusabfrage, verlorene Nachrichten, verspätete Freigabe und Wiederverbindung |

**Grenzen der Automatisierung.** Für TC-01 und TC-13 kann der Mock die Befehlsfolge prüfen. Ob der Servo seine Endposition erreicht, erfordert eine Beobachtung am Aufbau. Bei TC-17 und TC-18 prüft der PC die Protokollreaktion. Funkverbindung und Wiederanmeldung der realen Boards sind Hardwaretests. TC-20 erfordert eine Testperson, weil erst die Bedienung ohne Hilfestellung zeigt, ob die Rückmeldungen verständlich sind.

Das ursprüngliche Dokument mit den User Stories US-01 bis US-14 liegt nicht vor. US-03 und US-09 sind in den verfügbaren Unterlagen nicht beschrieben; US-10 bis US-14 sind im Testkonzept ausgenommen. Für diese Anforderungen wird daher keine Umsetzung behauptet. Details dazu stehen im [Anforderungsabgleich](docs/ANFORDERUNGEN.md).

## Bauen und testen

Die ausführliche [Anleitung zum Bauen und Übertragen der Firmware auf die Picos](docs/ANLEITUNG.md) beschreibt Voraussetzungen, Boardauswahl und Upload.

Zuerst die WLAN-Konfiguration anlegen und Zugangsdaten eintragen:

```powershell
Copy-Item include\parkhaus_secrets.h.example include\parkhaus_secrets.h
```

Danach Firmware bauen und Hosttests ausführen:

```powershell
python tools/build.py
python tools/test_host.py
```

Weitere Informationen zu Aufbau und Testdurchführung: [Hardwarecheckliste](docs/HARDWARETESTS.md). Architektur und Testergebnisse: [ARCHITEKTUR.md](docs/ARCHITEKTUR.md) und [TESTERGEBNISSE.md](docs/TESTERGEBNISSE.md).

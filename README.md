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

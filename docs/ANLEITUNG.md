# Anleitung: Projekt bauen und auf die Picos übertragen

Diese Anleitung beschreibt, wie die Firmware auf beide Boards übertragen wird. Die Rollen sind **Kasse** und **Schranke**; sie sind unabhängig davon, ob ein Board Pico W oder Pico 2 W ist.

## Voraussetzungen

- Git, Python 3 und Visual Studio Code
- PlatformIO IDE für VS Code oder PlatformIO Core
- WLAN mit 2,4 GHz, mit dem sich beide Picos verbinden können
- USB-Datenkabel für die Boards

Beim ersten Build werden PlatformIO-Plattform und Compiler heruntergeladen; dafür wird Internetzugang benötigt.

## 1. Repository vorbereiten

Repository in VS Code öffnen und im PlatformIO-Terminal ins Projektverzeichnis wechseln. WLAN-Konfiguration einmalig anlegen:

```powershell
Copy-Item include\parkhaus_secrets.h.example include\parkhaus_secrets.h
```

In `include\parkhaus_secrets.h` den WLAN-Namen und das Passwort eintragen. Diese Datei enthält vertrauliche Zugangsdaten und wird nicht ins Git-Repository aufgenommen. Die Datei nicht erneut aus der Vorlage kopieren, wenn sie bereits eingerichtet ist.

## 2. Boardmodell und Firmware wählen

Das Modell steht auf der Platine. `pico1-kasse` und `pico2-schranken` bezeichnen die Programmrollen, nicht die Generation des Boards.

| Boardmodell | Kassen-Firmware | Schranken-Firmware |
|---|---|---|
| Pico W / Pico WH (RP2040) | `pico1-kasse` | `pico2-schranken` |
| Pico 2 W (RP2350) | `pico1-kasse-rp2350` | `pico2-schranken-rp2350` |

Im PlatformIO-Terminal jeweils den passenden Build-Befehl ausführen:

```powershell
pio run -e pico1-kasse
pio run -e pico2-schranken
```

Für Pico 2 W stattdessen:

```powershell
pio run -e pico1-kasse-rp2350
pio run -e pico2-schranken-rp2350
```

Die gebauten Firmware-Dateien liegen unter `.pio\build\<Environment>\firmware.uf2`, wobei `<Environment>` durch den oben gewählten Namen zu ersetzen ist.

## 3. Firmware auf ein Board kopieren

Diese Schritte für jedes Board einzeln durchführen:

1. Das Board per USB-Datenkabel mit dem Computer verbinden.
2. BOOTSEL gedrückt halten, während das USB-Kabel eingesteckt wird; danach BOOTSEL loslassen. Das Board erscheint als USB-Laufwerk.
3. Die passende `firmware.uf2` aus dem Build-Ordner auf dieses Laufwerk kopieren. Der Laufwerksbuchstabe kann je nach Computer unterschiedlich sein.
4. Nach dem Kopieren startet das Board neu. Das USB-Laufwerk verschwindet üblicherweise wieder.

**Rollen richtig zuordnen:** `pico1-kasse` kommt auf das Board ohne Servo. `pico2-schranken` kommt auf das Board mit dem Servo. Bei Pico 2 W jeweils die dazugehörigen `-rp2350`-Dateien verwenden.

## 4. Start und Kontrolle

Beide Boards mit demselben WLAN verbinden und starten. Die Kasse zuerst starten; danach den Schranken-Pico. Die Kasse wird über WLAN gefunden. Auf der Schranken-Webseite prüfen, dass die Kasse als verbunden angezeigt wird.

Die IP-Adressen und Statusmeldungen lassen sich im seriellen Monitor ansehen:

```powershell
pio device list
pio device monitor --port COMx --baud 115200
```

`COMx` durch den Port des jeweiligen Boards ersetzen. Die von der Firmware ausgegebene IP-Adresse im Browser öffnen; DHCP-Adressen können sich ändern.

Vor der ersten Bewegung den Servo und die gemeinsame Masse korrekt anschließen und den Bewegungsbereich freihalten. Verkabelung, Inbetriebnahme und vollständige manuelle Prüfung sind in der [Hardwarecheckliste](HARDWARETESTS.md) beschrieben.

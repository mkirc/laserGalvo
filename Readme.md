# laserGalvo - technical writeup

## overview

Um beliebige Vektorgrafiken mit einem günstigen [Galvo-Aufbau](https://en.wikipedia.org/wiki/Mirror_galvanometer) zur Belichtung von fotosensitiven
Materialien  in eine XY- Ebene projezieren zu können, wird folgender prakmatischer Aufbau genutzt:

- Aus Vektorgrafik wird [Gcode](https://wikipedia.org/gcode) erzeugt
- Interpreter wandelt Gcode - Befehle in Bewegungs/Richtungs - Signale um
- Step/Dir Signale werden in Y/X - Werte zurückgerechnet
- X/Y - Werte werden als Spannungen an den Galvo - Treiber übergeben

Es werden folgende Designentscheidungen getroffen:

###Gcode als Transportmedium der Vektorgrafik
- **Vorteile** : Parser vorhanden, Kurven
- **Nachteile** : Viele Dialekte, Fehleranfälligkeit bei der Erzeugung

###Sequenzielles senden der Gcode Befehle über serielle Schnittstelle
- **Vorteile** : Gleicht geringen Speicher der verwendeten Microcontroller aus, verkleinert die Matrialliste
- **Nachteile** : Fehler bei der Übertragung möglich


Für das Senden der Gcode verwenden wir den [universalGcodeSender](https://winder.github.io/ugs_website/), wobei man je nach Betriebssystem 
schauen muss, welche Version mit Grbl v1.1 am besten funktioniert.

###Skizze

[full]: https://github.com/mkirc/laserGalvo/assets/full.png "Almost all you need" 

## grbl build for esp32

Die Esp firmware kommt vom [Grbl_Esp32](https://github.com/bdring/Grbl_Esp32) Projekt,
hierfür ein großes Dankeschön an die Menschen von [Grbl](https://github.com/gnea/grbl) und an bdring für den
Port.

Wir verwenden für die Hardware des Gcode interpreters ein [Esp32-devkit-v1](https://file.vishnumaiea.in/download/esp32/ESP32-Devkit-Pinout-Rev-12-4000p.png). Die GRBL firmware muss vor dem flashen noch
angepasst werden. Hier muss in [config.h](./gbl/Grbl_Esp32/config.h) die cpu (lies: pin) map CPU_MAP_ESP32 ausgewählt werden. 

```
// #define CPU_MAP_TEST_DRIVE
#define CPU_MAP_ESP32 
```

Die Einstellung holt die Firmware aus dem testmodus und mappt den für uns relevanten Output der Step/Dir 
Signale zu den folgenden PINs:

|xStep| GPIO 12|
|xDir|  GPIO 14|
|yStep| GPIO 26|
|yDir|  GPIO 15|

Zum flashen wird das [platformio](https://platformio.org/) tool verwendet.
Eine für die verwendeten Libraries und das Esp32-devkit angepasste platformio.ini liegt [hier](.gbl/platformio.ini).

Für eine Installation per ArduinoIDE ist der [Guide](https://github.com/bdring/Grbl_Esp32/wiki/Compiling-the-firmware) aus dem Grbl_Esp32 wiki sehr hilfreich.
(ARDUINO IDE HINWEIS: SPIFF MIN Partitionierung auswählen!)

Nach dem flashen kann man mit dem Befehl
```
platformio device monitor -p [PORT] -b 115200
```
überprüen ob alles funktioniert hat. Der serielle Monitor sollte eine Meldung der Form 
**Grbl vX.Xx ['$' for help]** und weitere Config-werte zeigen. Bei Problemen auch hier der Verweis ans Grbl-Wiki.

### Config

Es werden über das [Config Menü der Firmware](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration) folgende Einstellungen vorgenommen:

- Zur Vereinfachung der Berechnung von X/Y werden die Steps/mm **($101/102)** auf **1** gesetzt
- Damit die Step Signale sauber gemessen werden können, wird die Pulsbreite **($0)** auf **50** ms gesetzt 
- Es können zusätzlich Rate/Beschleunigung angepasst werden

Step/Dir werden als Pulse-Width-modulierte Signale der Form:

[step_dir]: https://github.com/mkirc/laserGalvo/assets/step_dir.png "ugly & phony"

Um die Ausgabe in für die Galvos verwertbare Signale umzurechnen, wird better & still phonyein Steppermotor in einem
[Arduino Pro Mini](https://cdn.sparkfun.com/assets/home_page_posts/1/9/4/7/ProMini16MHzv1.png) emuliert. Hier werden die Werte vom Esp32 ausgelesen und in X/Y Koordinaten umgerechnet:

[x_y]: https://github.com/mkirc/laserGalvo/x_y.png "better & still phony"


## map X/Y - step values to 12bit range of dac

Zunächst muss der Sketch [demo.ino](./poc/miniPro/src/demo.ino) auf das Arduino Board geflasht werden.
Hierfür wird ein herkömmlicher FT232 FTDi USB/UART Programmer verwendet. Die beiden Boards werden foglendermaßen verbunden:

|ft232  |Pro mini   |
|-------|-----------|
|rx     |tx         |
|tx     |rx         |
|gnd    |gnd        |
|vdd    |vdd        |

Zum Upload kann wiederum die ArduinoIDE oder das platformio tool verwendet werden.
Weiterhin müssen Esp32 und Arduino Pro Mini verbunden werden:

|Esp32  |Pro mini   |
|-------|-----------|
|12     |2          |
|14     |A2         |
|26     |3          |
|15     |A3         |

Die berechneten Werte werden über SPI an einen MCP 4822 DAC gesendet, der die Integerwerte auf
eine Spannung zwischen 0 und 4.096V mappt. Für das Interface wird die [MCP48x2](https://github.com/SweBarre/MCP48x2) Library von 
SweBarre genutzt, dem ebenso unser Dank gilt!

Für die Verbindung zwischen Arduino Pro Mini und MCP4822 gilt mit Referenz auf die [Dokumentation](http://ww1.microchip.com/downloads/en/DeviceDoc/20002249B.pdf) [pdf]

|Pro mini   |MCP4822    |
|-----------|-----------|
|5V         |Vdd        |
|PIN 13     |SCK        |
|PIN 10     |CS         |
|PIN 11     |SDI        |
|GND        |latch      |
|GND        |vss        |

VoutA/VoutB (vom MCP4822) werden mit X(-) bzw. Y(-) der Galvotreiberboards verbunden.

## TODO

Zur vollständigen Implementation des [ILDA]() - Standards, den die Galvotreiber erwarten, 
muss das Signal aus dem DAC noch auf +-5V gemappt und invertiert werden. Hierfür sollen Opamps verwendet werden.

Der Signalfluss Esp32 -> Pro mini -> DAC ist ziemlich hacky und störungsanfällig. Eine direktere Lösung über SPI
wäre wünschenswert.

Fotos / Videos vom Prozess



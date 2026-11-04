# Inkpot

**A paint program for Linux in which everything you draw stays an object.**

[![Version](https://img.shields.io/github/v/release/Neklor/Inkpot?label=Version&color=27AE60)](../../releases/latest)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-2C3E50)](LICENSE)
[![Donate with PayPal](https://img.shields.io/badge/Donate-PayPal-00457C?logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

*(Deutsche Fassung weiter unten — [zur deutschen Fassung](#inkpot-deutsch))*

![Inkpot – the main window with shapes, a brush stroke and the object list](docs/screenshot.png)

Inkpot is a paint program written in C++ with Qt 6. Everything you draw stays
an object of its own: brush strokes, lines, shapes, captions, filled areas and
pasted pictures can be selected, moved, turned, scaled, recoloured and deleted
at any time, long after they were drawn. The project format keeps them that
way, so a drawing never hardens into a flat picture.

> ### ☕ Inkpot is free
>
> If you find it useful, you can support its development with a donation.
>
> [![Donate with PayPal](https://img.shields.io/badge/Donate%20with%20PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

## Features

**Tools**
* Selection, brush, straight line, shapes, bucket fill, eraser and text
* Shapes: rectangle, ellipse, triangle, heart and star, solid or outlined.
  Hold Shift while dragging to keep the sides even
* The eraser works *inside* objects: the erased area is masked out while the
  original shape is kept in the project file
* The bucket fills an enclosed area the way a raster paint program does, but
  works out that area on the rendered picture of the whole canvas. An area
  enclosed by several overlapping objects is therefore filled as one, and the
  result becomes an object of its own
* Captions can be edited again with a double click

**Objects**
* Everything drawn or pasted becomes a separate object
* Select, move, scale through eight handles, turn with the round handle above
  the object, recolour, delete
* Turning and scaling work in the object's own axes, so a turned object still
  grows along its own edges instead of being distorted. Hold Shift while
  turning for 15 degree steps
* Rubber band selection for several objects at once
* Scaling a caption changes its font size rather than distorting it, and the
  size follows live in the toolbar

**Object list**
* A dock on the right lists every object, topmost first, with its colour
* Picking a row selects the object on the canvas and the other way round;
  Ctrl picks several, Shift a block
* It can be hidden through View > Object List, and the window remembers that

**History**
* Undo and redo for drawing, filling, erasing, deleting, moving, turning,
  scaling, recolouring, editing a caption and resizing the canvas
* One drag is one step: a whole stroke of the eraser comes back at once

**Canvas**
* Pinned to the top left corner, resized with handles on its right and bottom
  edge or by typing the size in the status bar
* Lockable aspect ratio, with the current ratio shown as 4:3, 16:9 and so on

**Files**
* Project format `*.ipot` (JSON) that keeps every object editable
* Opens HEIF and HEIC pictures, the format phones write by default
* Exports to the picture formats the installed Qt image plugins can write,
  PNG and JPEG always among them
* Copy to clipboard, paste, print

## Installing

**Debian, Ubuntu and Linux Mint:** a `.deb` package hangs on the
[latest release](../../releases/latest). Download it and double-click it, or:

```bash
sudo apt install ./inkpot_1.0.0_amd64.deb
```

It brings the menu entry, the icon and the file type for `*.ipot` with it. The
package is built against the Qt version of Ubuntu 24.04, so it installs on
that and anything newer.

A package on **Flathub** is being considered, which would also make Inkpot
findable in the software centres of Linux Mint, Fedora and others. Until then,
everything else is built from source.

## Building

Requirements (Debian, Ubuntu, Linux Mint):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev \
                 qt6-base-dev-tools qt6-l10n-tools libheif-dev \
                 qt6-image-formats-plugins libgl1-mesa-dev
```

Two of these are optional. `qt6-l10n-tools` provides `lupdate` and `lrelease`
for the translations; without it Inkpot still builds, it simply runs
untranslated. `libheif-dev` adds HEIF and HEIC support; without it those files
cannot be opened. `qt6-image-formats-plugins` widens the list of picture
formats Qt can read and write, TIFF and WebP among them.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/inkpot
```

Installing system wide:

```bash
sudo cmake --install build
sudo update-mime-database /usr/local/share/mime
sudo update-desktop-database /usr/local/share/applications
```

This installs the program, the desktop entry, the application icon, the
AppStream metadata, the MIME type for `*.ipot` and the translations.

## Keys and mouse

The program lists these under **Help > Keyboard Shortcuts** as well, gathered
from the menus themselves so the list stays current.

| Action | Input |
| --- | --- |
| Selection | `S` |
| Brush | `B` |
| Line | `L` |
| Eraser | `E` |
| Bucket fill | `F` |
| Shapes | `R` |
| Text | `T` |
| Edit a caption later | double click with the selection tool |
| Recolour objects | select them, then the colour button |
| Undo / redo | `Ctrl+Z` / `Ctrl+Shift+Z` |
| Delete selection | `Del` |
| Select all | `Ctrl+A` |
| Zoom | `Ctrl` + mouse wheel, `Ctrl++`, `Ctrl+-` |
| Actual size / fit in window | `Ctrl+0` / `Ctrl+9` |
| Pan the view | drag with the middle mouse button |
| Turn in 15 degree steps | hold `Shift` while dragging the turning handle |
| Keep a shape even | hold `Shift` while drawing it |

## Translations

The strings in the sources are English; translations live in
[`translations/`](translations/) as Qt Linguist catalogues. This is the usual
mechanism for Qt applications and is understood by translation platforms such
as Weblate.

Editing a translation — either with `linguist6 translations/inkpot_de.ts` or in
a text editor, the files are readable XML:

```xml
<message>
    <source>&amp;Save</source>
    <translation>&amp;Speichern</translation>
</message>
```

Adding a language:

```bash
cp translations/inkpot_de.ts translations/inkpot_fr.ts
# set <TS ... language="fr_FR"> and list the file under
# INKPOT_TS_FILES in CMakeLists.txt
cmake --build build
```

After changing the sources,

```bash
cmake --build build --target update_translations
```

pulls new and changed strings into every `.ts` file.

## Packaging

| File | Purpose |
| --- | --- |
| `data/io.github.neklor.Inkpot.desktop` | application menu entry |
| `data/io.github.neklor.Inkpot.metainfo.xml` | AppStream data for software centres |
| `data/io.github.neklor.Inkpot.xml` | MIME type for `*.ipot` |
| `data/icons/hicolor/scalable/apps/…svg` | application icon |
| `flatpak/io.github.neklor.Inkpot.yml` | Flatpak manifest |
| `tools/build-deb.sh` | builds the Debian package |

Building a Flatpak locally:

```bash
flatpak install flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
flatpak-builder --user --install --force-clean build-flatpak \
    flatpak/io.github.neklor.Inkpot.yml
flatpak run io.github.neklor.Inkpot
```

The screenshot shown at the top doubles as the picture in software centres;
it lives in `docs/screenshot.png` and the AppStream file points at it.
`tools/make-social-preview.py` builds the card GitHub shows when the project
is linked somewhere.

## Project structure

| Path | Contents |
| --- | --- |
| `src/main.cpp` | program start, translations, command line |
| `src/MainWindow.*` | menus, toolbar, status bar, file operations |
| `src/CanvasScene.*` | canvas, object handling, project format |
| `src/CanvasView.*` | view, tools, zoom |
| `src/ResizeHandle.*` | handles for canvas and objects |
| `src/StrokeItem.*` | object: brush stroke and line |
| `src/FillItem.*` | object: filled area |
| `src/ShapeItem.*` | object: shape |
| `src/HeifImage.*` | reading HEIF and HEIC pictures |
| `src/FloodFill.*` | working out the area a bucket fill covers |
| `src/ImageItem.*` | object: image |
| `src/TextItem.*` | object: caption |
| `src/InkObject.*` | shared object interface including the erase mask |
| `src/Icons.*` | icons drawn at run time |
| `src/NewCanvasDialog.*` | dialog for a custom canvas size |
| `src/ShortcutsDialog.*` | list of the keyboard shortcuts |
| `src/ObjectListDock.*` | the object list on the right |
| `src/Commands.*` | undo commands |
| `translations/` | translation catalogues |
| `data/` | icon, desktop entry, AppStream and MIME data |
| `flatpak/` | Flatpak manifest |
| `tools/` | helper scripts that are not part of the program |
| `packaging/` | the maintainer scripts of the Debian package |
| `docs/` | screenshot and the social preview card |

## Contributing

Sources, comments and the strings in the user interface are English so the
project stays open to contributors and translators. Commit messages are
written in English.

Changes are recorded in [CHANGELOG.md](CHANGELOG.md). The version number lives
in [CMakeLists.txt](CMakeLists.txt) and is handed from there to the program and
the package data.

## Supporting the project

Inkpot is written in my spare time and is free. If you find it useful, you can
support its development with a donation:

[![Donate with PayPal](https://img.shields.io/badge/Donate%20with%20PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

The **Sponsor** button at the top of the project page leads to the same link.

Donations are voluntary support for a private spare-time project. They are not
an order and establish no claim to a particular feature or a particular
schedule — but I will say openly what I am working on.

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).

Copyright © 2026 Christoph Krogmann

---

# Inkpot (Deutsch)

**Ein Malprogramm für Linux, in dem alles Gemalte ein Objekt bleibt.**

[![Version](https://img.shields.io/github/v/release/Neklor/Inkpot?label=Version&color=27AE60)](../../releases/latest)
[![Lizenz: GPL-3.0](https://img.shields.io/badge/Lizenz-GPL--3.0-2C3E50)](LICENSE)
[![Mit PayPal spenden](https://img.shields.io/badge/Spenden-PayPal-00457C?logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

*(English version above — [jump to English](#inkpot))*

![Inkpot – das Hauptfenster mit Formen, einem Pinselstrich und der Objektliste](docs/screenshot.png)

Inkpot ist ein Malprogramm in C++ mit Qt 6. Alles Gemalte bleibt ein eigenes
Objekt: Pinselstriche, Linien, Formen, Beschriftungen, gefüllte Flächen und
eingefügte Bilder lassen sich jederzeit auswählen, verschieben, drehen,
skalieren, umfärben und löschen — auch lange nachdem sie entstanden sind. Das
Projektformat hält sie so; eine Zeichnung erstarrt nie zu einem flachen Bild.

> ### ☕ Inkpot ist kostenlos
>
> Wenn es dir gefällt, kannst du die Weiterentwicklung mit einer Spende
> unterstützen.
>
> [![Mit PayPal spenden](https://img.shields.io/badge/Mit%20PayPal%20spenden-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

## Funktionen

**Werkzeuge**
* Auswahl, Pinsel, gerade Linie, Formen, Farbeimer, Radiergummi und Text
* Formen: Rechteck, Ellipse, Dreieck, Herz und Stern, gefüllt oder als Umriss.
  Mit gehaltener Umschalttaste bleiben die Seiten gleich lang
* Der Radiergummi wirkt *innerhalb* von Objekten: Die radierte Stelle wird
  ausgespart, die ursprüngliche Form bleibt in der Projektdatei erhalten
* Der Farbeimer füllt geschlossene Flächen wie in einem Pixel-Malprogramm,
  ermittelt die Fläche aber auf dem gezeichneten Bild der gesamten
  Arbeitsfläche. Eine Fläche, die mehrere überlappende Objekte gemeinsam
  einschließen, wird daher als eine gefüllt; das Ergebnis ist wieder ein
  eigenes Objekt
* Beschriftungen lassen sich per Doppelklick wieder bearbeiten

**Objekte**
* Alles Gemalte oder Eingefügte wird ein eigenes Objekt
* Auswählen, verschieben, über acht Anfasspunkte skalieren, über den runden
  Anfasser oberhalb drehen, umfärben, löschen
* Drehen und Skalieren arbeiten in den Achsen des Objekts: ein gedrehtes
  Objekt wächst weiter entlang seiner eigenen Kanten statt verzerrt zu werden.
  Mit gehaltener Umschalttaste rastet die Drehung in 15-Grad-Schritten
* Gummiband-Auswahl für mehrere Objekte auf einmal
* Beim Skalieren einer Beschriftung ändert sich die Schriftgröße statt einer
  Verzerrung, und der Wert läuft live in der Werkzeugleiste mit

**Objektliste**
* Eine Leiste rechts führt alle Objekte auf, das oberste zuerst, mit ihrer Farbe
* Eine Zeile anzuklicken wählt das Objekt auf der Fläche aus und umgekehrt;
  mit Strg mehrere, mit Umschalt ein Block
* Über Ansicht > Objektliste ausblendbar; das Fenster merkt sich die Einstellung

**Verlauf**
* Rückgängig und Wiederholen für Malen, Füllen, Radieren, Löschen, Verschieben,
  Drehen, Skalieren, Umfärben, Textänderungen und Größenänderungen der Fläche
* Ein Ziehen ist ein Schritt: ein ganzer Radierer-Zug kommt auf einmal zurück

**Arbeitsfläche**
* Liegt fest in der linken oberen Ecke, veränderbar über die Anfasspunkte an
  rechter und unterer Kante oder durch Eingabe der Größe in der Fußzeile
* Feststellbares Seitenverhältnis, mit Anzeige als 4:3, 16:9 und so weiter

**Dateien**
* Projektformat `*.ipot` (JSON), das jedes Objekt bearbeitbar hält
* Öffnet HEIF- und HEIC-Bilder, das Format, das Telefone standardmäßig schreiben
* Export in die Bildformate, die die installierten Qt-Bildmodule schreiben
  können; PNG und JPEG sind immer dabei
* In die Zwischenablage kopieren, einfügen, drucken

## Installieren

**Debian, Ubuntu und Linux Mint:** An der
[neuesten Veröffentlichung](../../releases/latest) hängt ein `.deb`-Paket.
Herunterladen und doppelklicken, oder im Terminal:

```bash
sudo apt install ./inkpot_1.0.0_amd64.deb
```

Menüeintrag, Programmsymbol und der Dateityp für `*.ipot` kommen mit. Das
Paket ist gegen die Qt-Fassung von Ubuntu 24.04 gebaut und lässt sich dort und
auf allem Neueren installieren.

Ein Paket bei **Flathub** wird erwogen; damit wäre Inkpot auch in der
Anwendungsverwaltung von Linux Mint, Fedora und anderen zu finden. Bis dahin
gilt für alles Übrige: aus dem Quellcode bauen.

## Bauen

Voraussetzungen (Debian, Ubuntu, Linux Mint):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev \
                 qt6-base-dev-tools qt6-l10n-tools libheif-dev \
                 qt6-image-formats-plugins libgl1-mesa-dev
```

Zwei davon sind freiwillig. `qt6-l10n-tools` liefert `lupdate` und `lrelease`
für die Übersetzungen; ohne das Paket baut Inkpot trotzdem, läuft dann aber
unübersetzt. `libheif-dev` bringt die HEIF- und HEIC-Unterstützung; ohne das
Paket lassen sich solche Dateien nicht öffnen. `qt6-image-formats-plugins`
erweitert die Bildformate, die Qt lesen und schreiben kann, darunter TIFF und
WebP.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/inkpot
```

Systemweit installieren:

```bash
sudo cmake --install build
sudo update-mime-database /usr/local/share/mime
sudo update-desktop-database /usr/local/share/applications
```

Damit kommen Programm, Menüeintrag, Programmsymbol, AppStream-Daten, der
MIME-Typ für `*.ipot` und die Übersetzungen mit.

## Tasten und Maus

Das Programm zeigt sie auch unter **Hilfe > Tastenkürzel** an; die Liste wird
aus den Menüs selbst zusammengestellt und bleibt damit aktuell.

| Aktion | Eingabe |
| --- | --- |
| Auswahl | `S` |
| Pinsel | `B` |
| Linie | `L` |
| Radiergummi | `E` |
| Farbeimer | `F` |
| Formen | `R` |
| Text | `T` |
| Beschriftung später ändern | Doppelklick mit dem Auswahlwerkzeug |
| Objekte umfärben | auswählen, dann der Farbknopf |
| Rückgängig / Wiederholen | `Strg+Z` / `Strg+Umschalt+Z` |
| Auswahl löschen | `Entf` |
| Alles auswählen | `Strg+A` |
| Zoomen | `Strg` + Mausrad, `Strg++`, `Strg+-` |
| Originalgröße / Einpassen | `Strg+0` / `Strg+9` |
| Ansicht verschieben | Ziehen mit der mittleren Maustaste |
| In 15-Grad-Schritten drehen | `Umschalt` beim Ziehen des Dreh-Anfassers |
| Form gleichseitig halten | `Umschalt` beim Aufziehen der Form |

## Übersetzungen

Die Texte im Quelltext sind englisch; Übersetzungen liegen unter
[`translations/`](translations/) als Qt-Linguist-Kataloge. Das ist das übliche
Verfahren für Qt-Anwendungen und wird von Übersetzungsplattformen wie Weblate
unterstützt.

Eine Übersetzung bearbeiten — entweder mit `linguist6 translations/inkpot_de.ts`
oder im Texteditor, die Dateien sind lesbares XML:

```xml
<message>
    <source>&amp;Save</source>
    <translation>&amp;Speichern</translation>
</message>
```

Eine neue Sprache anlegen:

```bash
cp translations/inkpot_de.ts translations/inkpot_fr.ts
# <TS ... language="fr_FR"> setzen und die Datei in CMakeLists.txt
# bei INKPOT_TS_FILES eintragen
cmake --build build
```

Nach Änderungen am Quelltext holt

```bash
cmake --build build --target update_translations
```

neue und geänderte Texte in alle `.ts`-Dateien.

## Veröffentlichung

| Datei | Zweck |
| --- | --- |
| `data/io.github.neklor.Inkpot.desktop` | Eintrag im Anwendungsmenü |
| `data/io.github.neklor.Inkpot.metainfo.xml` | AppStream-Daten für Software-Zentren |
| `data/io.github.neklor.Inkpot.xml` | MIME-Typ für `*.ipot` |
| `data/icons/hicolor/scalable/apps/…svg` | Anwendungssymbol |
| `flatpak/io.github.neklor.Inkpot.yml` | Flatpak-Manifest |
| `tools/build-deb.sh` | Baut das Debian-Paket |

Flatpak lokal bauen:

```bash
flatpak install flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
flatpak-builder --user --install --force-clean build-flatpak \
    flatpak/io.github.neklor.Inkpot.yml
flatpak run io.github.neklor.Inkpot
```

Das Bildschirmfoto oben dient zugleich als Bild in den Software-Zentren; es
liegt in `docs/screenshot.png`, die AppStream-Datei zeigt darauf.
`tools/make-social-preview.py` erzeugt die Vorschaukarte, die GitHub zeigt,
wenn das Projekt irgendwo verlinkt wird.

## Projektstruktur

| Pfad | Inhalt |
| --- | --- |
| `src/main.cpp` | Programmstart, Übersetzungen, Kommandozeile |
| `src/MainWindow.*` | Menüs, Werkzeugleiste, Fußzeile, Dateioperationen |
| `src/CanvasScene.*` | Arbeitsfläche, Objektverwaltung, Projektformat |
| `src/CanvasView.*` | Ansicht, Werkzeuge, Zoom |
| `src/ResizeHandle.*` | Anfasspunkte für Arbeitsfläche und Objekte |
| `src/StrokeItem.*` | Objekt: Pinselstrich und Linie |
| `src/FillItem.*` | Objekt: gefüllte Fläche |
| `src/ShapeItem.*` | Objekt: Form |
| `src/HeifImage.*` | Liest HEIF- und HEIC-Bilder |
| `src/FloodFill.*` | Ermittelt die Fläche, die der Farbeimer füllt |
| `src/ImageItem.*` | Objekt: Bild |
| `src/TextItem.*` | Objekt: Beschriftung |
| `src/InkObject.*` | Gemeinsame Objekt-Schnittstelle samt Radier-Maske |
| `src/Icons.*` | Zur Laufzeit gezeichnete Symbole |
| `src/NewCanvasDialog.*` | Dialog für eine eigene Flächengröße |
| `src/ShortcutsDialog.*` | Übersicht der Tastenkürzel |
| `src/ObjectListDock.*` | Die Objektliste rechts |
| `src/Commands.*` | Befehle für den Verlauf |
| `translations/` | Übersetzungskataloge |
| `data/` | Symbol, Desktop-Eintrag, AppStream- und MIME-Daten |
| `flatpak/` | Flatpak-Manifest |
| `tools/` | Hilfsskripte, die nicht zum Programm gehören |
| `packaging/` | Die Betreuerskripte des Debian-Pakets |
| `docs/` | Bildschirmfoto und die Vorschaukarte |

## Mitarbeit

Quelltext, Kommentare und die Texte in der Oberfläche sind englisch, damit das
Projekt für Mitwirkende und Übersetzer offen bleibt. Commit-Nachrichten werden
auf Englisch geschrieben.

Änderungen werden in [CHANGELOG.md](CHANGELOG.md) festgehalten. Die
Versionsnummer steht in [CMakeLists.txt](CMakeLists.txt) und wird von dort an
Programm und Paketdaten weitergereicht.

## Projekt unterstützen

Inkpot entsteht in meiner Freizeit und ist kostenlos. Wenn es euch gefällt,
könnt ihr die Weiterentwicklung mit einer Spende unterstützen:

[![Mit PayPal spenden](https://img.shields.io/badge/Mit%20PayPal%20spenden-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/CKrogmann)

Über die Schaltfläche **Sponsor** oben auf der Projektseite geht es zum
gleichen Link.

Spenden sind freiwillige Unterstützung eines privaten Freizeitprojekts. Sie
sind keine Bestellung und begründen keinen Anspruch auf eine bestimmte Funktion
oder einen Zeitplan — ich sage aber offen, woran ich arbeite.

## Lizenz

GNU General Public License v3.0 — siehe [LICENSE](LICENSE).

Copyright © 2026 Christoph Krogmann

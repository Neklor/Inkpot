# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- In the list of shortcuts, the mouse gestures were set flush right and
  wrapped, which left their lines starting at a different place each time.
  They read from the left now, in a held back tone, and a key named inside
  such a line is drawn as a key: Ctrl carries a cap of its own while the
  mouse wheel beside it stays text. The entry for undo also said what it
  would undo, so it changed with every step; it gives its plain name there.

### Added

- Objects can be given a name of their own. Double click a row in the object
  list, press F2, or pick Rename Object from the Edit menu or from the
  context menu of the list; an empty name puts back what the object is. The
  name is kept in the project file, and a file written by an older version
  simply has none.

- Objects can be moved through the stack, which decides who covers whom. Two
  buttons below the object list, and the entries Bring Forward and Send
  Backward in the Edit menu on Ctrl+Page Up and Ctrl+Page Down. Several
  objects move at once and keep their order among themselves, and a row can
  also be dragged straight to where it belongs.

## [1.1.3] - 2026-11-08

### Fixed

- Filled areas of shapes with straight edges came out bent when they were
  made much larger. Tracing the outline with curves, added in 1.1.2, was
  applied to every part of it that was not a sharp corner, so the arms of a
  star sagged and its points drooped. Only the round parts are traced now: a
  straight edge of the drawing survives the fill as one long segment, a round
  one as a chain of short segments, and the two are told apart by that. A
  star keeps straight arms and sharp points, a circle stays round.

### Changed

- The shape tool's dropdown is easier to hit: larger pictures in the menu,
  taller rows and a wider arrow beside the button. It was the one control in
  the window that had to be aimed at, which a test on a touchscreen made
  plain.

- The Debian package lists the Qt 6 libraries under both the name Ubuntu
  uses and the name Debian uses, whichever of the two the machine it was
  built on happens to use. Until now only one direction was covered, so a
  package built on Debian would not have installed on Ubuntu or Mint.

## [1.1.2] - 2026-11-06

### Fixed

- Filled areas had jagged edges once they were scaled up. The bucket fill
  took the shape of the area from the canvas pixel by pixel, so every
  slanted edge was a staircase that grew along with the object. The area is
  now found on a picture drawn three times larger, and its outline is
  straightened and then traced with curves, so slanted edges stay clean and
  round edges stay round. Areas filled with an earlier version keep the
  outline they were saved with.

  Filling takes longer for it: about a quarter of a second on a canvas of
  1564 by 1020 pixels, where it used to be hundredths. A filled area also
  still stops a little short of a sharp point, because that is where the
  enclosed area really ends - about three pixels at the point of a star,
  down from four and a half.

## [1.1.1] - 2026-11-06

### Fixed

- The Debian package would not install on Debian 13. Ubuntu and Linux Mint
  carry the 64-bit `time_t` transition in the names of the Qt 6 library
  packages, Debian kept the plain ones, and the package was built with the
  Ubuntu names alone. Both spellings are now accepted. The program itself is
  unchanged.

## [1.1.0] - 2026-11-05

### Added

- French and Spanish translations. Both were machine translated and then read
  through by the author, who is not a native speaker of either, so the README
  asks for corrections.
- A Debian package, built from the existing install rules with CPack through
  `tools/build-deb.sh`, and a `PKGBUILD` for Arch and the distributions based
  on it. Neither has to be published anywhere first.

### Fixed

- Access keys in the German menus. Exportieren and Einfügen both claimed E,
  Originalgröße and Objektliste both O; a duplicate within one menu leaves
  both entries unreachable by keyboard.
- The export dialog offered TIFF and WebP whether or not Qt could write them.

## [1.0.0] - 2026-11-04

First release.

### Added

- Canvas pinned to the top left corner, resized with handles on its right and
  bottom edge or through the fields in the status bar, with a lockable aspect
  ratio and a live ratio display.
- Object handling: brush strokes, straight lines, captions and pasted images
  are objects of their own and can be selected, moved, scaled through eight
  handles, turned with a round handle above them, recoloured and deleted.
  Rotation and scale are kept as separate values in the object and combined in
  that order, so a turned object still grows along its own axes instead of
  being distorted. Holding Shift turns in 15 degree steps.
- Tools for selection, brush, line, eraser, bucket fill and text. The eraser
  works inside objects by masking the erased area, so the original shape is
  preserved in the project file.
- The bucket fill determines its area on the rendered picture of the whole
  canvas, so an area enclosed by several overlapping objects is filled as one.
  The result is an object of its own and can be recoloured, scaled, erased
  into and deleted like any other.
- Font size field that follows the selected caption; dragging its handles
  changes the font size instead of distorting the text.
- File menu with new (preset and custom sizes), open, save, save as, export,
  copy to clipboard, paste, print and quit. The export dialog offers the
  picture formats the installed Qt image plugins can actually write, rather
  than a fixed list.
- Project format `*.ipot` that keeps every object editable.
- Shape tool with rectangle, ellipse, triangle, heart and star, drawn solid or
  as an outline. Holding Shift keeps the sides even.
- Opens HEIF and HEIC pictures through libheif, the format phones write by
  default. The support is optional at build time.
- Object list on the right: every object with its colour, topmost first.
  Picking a row selects the object and the other way round, with Ctrl for
  several and Shift for a block. It hides through the View menu and the window
  remembers the layout between runs.
- Undo and redo covering drawing, filling, erasing, deleting, moving, turning,
  scaling, recolouring, editing a caption and resizing the canvas. One drag is
  one step.
- Tool icons drawn at run time, so the project ships no image files.
- Desktop integration: desktop entry, AppStream metadata, application icon and
  a MIME type for `*.ipot`, plus a Flatpak manifest.
- German translation through Qt Linguist catalogues. When the LinguistTools
  CMake package is absent, the build falls back to calling `lrelease` directly,
  so the smaller qt6-l10n-tools package is enough.
- Help menu with a list of the keyboard shortcuts, gathered from the menus and
  the toolbar so it cannot fall out of step, and an about dialog carrying
  version, author, licence and homepage.

[Unreleased]: https://github.com/Neklor/Inkpot/compare/v1.1.3...HEAD
[1.1.3]: https://github.com/Neklor/Inkpot/releases/tag/v1.1.3
[1.1.2]: https://github.com/Neklor/Inkpot/releases/tag/v1.1.2
[1.1.1]: https://github.com/Neklor/Inkpot/releases/tag/v1.1.1
[1.1.0]: https://github.com/Neklor/Inkpot/releases/tag/v1.1.0
[1.0.0]: https://github.com/Neklor/Inkpot/releases/tag/v1.0.0

# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/Neklor/Inkpot/compare/v1.1.1...HEAD
[1.1.1]: https://github.com/Neklor/Inkpot/releases/tag/v1.1.1
[1.1.0]: https://github.com/Neklor/Inkpot/releases/tag/v1.1.0
[1.0.0]: https://github.com/Neklor/Inkpot/releases/tag/v1.0.0

#pragma once

#include <QIcon>

class QColor;

// Every tool icon is drawn at run time. That way the project needs no image
// files and the icons follow the text colour of the current theme.
namespace Icons {

QIcon pointer(const QColor &color);   // Selection: mouse pointer

// Brush whose bristles carry the currently chosen drawing colour. The
// outline keeps very light colours visible on a light background.
QIcon brush(const QColor &accent, const QColor &outline);
QIcon line(const QColor &color);      // Line

// Two-tone eraser in the style of a classic Pelikan block. The colour given
// is used for the outline, so it follows the theme.
QIcon eraser(const QColor &outline);
QIcon text(const QColor &color);      // Text

// Paint bucket pouring the currently chosen colour. As with the brush, the
// outline follows the theme so the icon reads on any background.
QIcon bucket(const QColor &accent, const QColor &outline);
QIcon plus(const QColor &color);      // New
QIcon folder(const QColor &color);    // Open

// Aspect ratio lock: closed and open padlock.
QIcon lockClosed(const QColor &color);
QIcon lockOpen(const QColor &color);

}   // namespace Icons

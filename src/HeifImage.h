#pragma once

#include <QString>

class QImage;

// Reading HEIF and HEIC pictures, the format phones write by default.
//
// Qt brings no reader for it, so libheif does the work. The support is
// optional at build time: without libheif the program still builds and simply
// reports that it cannot open such files.
namespace HeifImage {

// True when the program was built with libheif.
bool isSupported();

// Does the file name look like a HEIF picture?
bool looksLikeHeif(const QString &path);

// Reads the main picture out of the file. On failure the image is null and
// error says what went wrong.
QImage load(const QString &path, QString *error = nullptr);

}   // namespace HeifImage

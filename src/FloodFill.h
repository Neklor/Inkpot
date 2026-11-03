#pragma once

#include <QPainterPath>
#include <QPoint>

class QImage;

// Finding the area a bucket fill should cover.
//
// The area is determined on the rendered picture of the whole canvas, not on a
// single object. That is what makes the fill work across objects: an area
// enclosed by several overlapping strokes is still recognised as one area.
namespace FloodFill {

// Connected area around start whose colour matches the colour at start within
// tolerance, as a path in image coordinates.
//
// tolerance is the largest difference per colour channel that still counts as
// the same colour. grow widens the area by that many pixels so the fill tucks
// under the antialiased edges that bound it instead of leaving a pale seam.
QPainterPath regionAt(const QImage &image, const QPoint &start, int tolerance = 24, int grow = 1);

}   // namespace FloodFill

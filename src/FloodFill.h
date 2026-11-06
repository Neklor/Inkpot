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

// Replaces the stair steps of a pixel outline with straight runs.
//
// A point is dropped when it lies closer than tolerance to the line between
// the points that remain on either side of it, so a run of single-pixel steps
// collapses into one straight edge while a real corner, which lies far from
// that line, is kept. tolerance is measured in the coordinates of the path.
QPainterPath straightened(const QPainterPath &path, qreal tolerance);

// Traces the outline with curves instead of keeping it a chain of segments.
//
// Where the outline turns gently it is replaced by curves that run through
// the middles of the segments, which is what makes a round edge round again.
// Where it turns by more than cornerDegrees the point is kept as it is, so
// the tip of a star stays a tip. This is the same idea as the tracing that
// turns a scanned drawing into a vector picture.
QPainterPath traced(const QPainterPath &path, qreal cornerDegrees = 60.0);

}   // namespace FloodFill

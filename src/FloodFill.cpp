#include "FloodFill.h"

#include <QBitmap>
#include <QImage>
#include <QLineF>
#include <QPolygonF>
#include <QRegion>
#include <QStack>
#include <QVector>

namespace {

inline bool matches(QRgb a, QRgb b, int tolerance)
{
    return qAbs(qRed(a)   - qRed(b))   <= tolerance
        && qAbs(qGreen(a) - qGreen(b)) <= tolerance
        && qAbs(qBlue(a)  - qBlue(b))  <= tolerance
        && qAbs(qAlpha(a) - qAlpha(b)) <= tolerance;
}

// Widens the area by one pixel in each direction, repeated steps times.
void dilate(QVector<bool> &inside, int width, int height, int steps)
{
    for (int step = 0; step < steps; ++step) {
        const QVector<bool> previous = inside;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (previous.at(y * width + x))
                    continue;
                const bool neighbour =
                    (x > 0          && previous.at(y * width + x - 1))
                 || (x < width - 1  && previous.at(y * width + x + 1))
                 || (y > 0          && previous.at((y - 1) * width + x))
                 || (y < height - 1 && previous.at((y + 1) * width + x));
                if (neighbour)
                    inside[y * width + x] = true;
            }
        }
    }
}

qreal distanceToSegment(const QPointF &point, const QPointF &from, const QPointF &to)
{
    const QPointF along = to - from;
    const qreal length2 = along.x() * along.x() + along.y() * along.y();
    if (length2 <= 0.0)
        return QLineF(point, from).length();

    const qreal t = qBound(0.0, QPointF::dotProduct(point - from, along) / length2, 1.0);
    return QLineF(point, from + t * along).length();
}

// Ramer-Douglas-Peucker over one open chain: the point furthest from the line
// between the ends is kept if it lies further away than tolerance, and both
// halves are then looked at the same way.
void keepCorners(const QPolygonF &points, int first, int last, qreal tolerance,
                 QVector<bool> &keep)
{
    if (last <= first + 1)
        return;

    qreal furthest = 0.0;
    int at = -1;
    for (int i = first + 1; i < last; ++i) {
        const qreal distance = distanceToSegment(points.at(i), points.at(first), points.at(last));
        if (distance > furthest) {
            furthest = distance;
            at = i;
        }
    }
    if (at < 0 || furthest <= tolerance)
        return;

    keep[at] = true;
    keepCorners(points, first, at, tolerance, keep);
    keepCorners(points, at, last, tolerance, keep);
}

QPolygonF straightenRing(QPolygonF ring, qreal tolerance)
{
    if (!ring.isEmpty() && ring.first() == ring.last())
        ring.removeLast();

    const int count = ring.size();
    if (count < 4)
        return ring;

    // A closed ring has no ends to start from. The point furthest from the
    // first one serves as the second anchor, which splits the ring into two
    // chains; both anchors sit on the outline, so they are safe to keep.
    int opposite = 0;
    qreal widest = -1.0;
    for (int i = 1; i < count; ++i) {
        const qreal distance = QLineF(ring.at(0), ring.at(i)).length();
        if (distance > widest) {
            widest = distance;
            opposite = i;
        }
    }

    QVector<bool> keep(count, false);
    keep[0] = true;
    keep[opposite] = true;
    keepCorners(ring, 0, opposite, tolerance, keep);

    QPolygonF back;
    for (int i = opposite; i < count; ++i)
        back << ring.at(i);
    back << ring.at(0);

    QVector<bool> keepBack(back.size(), false);
    keepCorners(back, 0, back.size() - 1, tolerance, keepBack);
    for (int i = 1; i < back.size() - 1; ++i) {
        if (keepBack.at(i))
            keep[opposite + i] = true;
    }

    QPolygonF straight;
    for (int i = 0; i < count; ++i) {
        if (keep.at(i))
            straight << ring.at(i);
    }
    return straight;
}

// How sharply the outline turns at one point, in degrees. Zero means the two
// segments continue in a straight line.
qreal turnAt(const QPolygonF &ring, int at)
{
    const int count = ring.size();
    const QPointF before = ring.at((at - 1 + count) % count);
    const QPointF here = ring.at(at);
    const QPointF after = ring.at((at + 1) % count);

    const QLineF in(before, here);
    const QLineF out(here, after);
    if (in.length() <= 0.0 || out.length() <= 0.0)
        return 0.0;

    qreal turn = qAbs(in.angleTo(out));
    if (turn > 180.0)
        turn = 360.0 - turn;
    return turn;
}

QPointF middle(const QPointF &a, const QPointF &b)
{
    return QPointF((a.x() + b.x()) / 2.0, (a.y() + b.y()) / 2.0);
}

void traceRing(const QPolygonF &ring, qreal cornerDegrees, QPainterPath &into)
{
    const int count = ring.size();
    if (count < 3)
        return;

    QVector<bool> corner(count, false);
    int corners = 0;
    for (int i = 0; i < count; ++i) {
        if (turnAt(ring, i) > cornerDegrees) {
            corner[i] = true;
            ++corners;
        }
    }

    // Curves run from the middle of one segment to the middle of the next,
    // with the point between them pulling the curve towards itself. A chain
    // of points on a straight line stays straight under that rule, so only
    // the places that really bend are rounded.
    if (corners == 0) {
        into.moveTo(middle(ring.at(count - 1), ring.at(0)));
        for (int i = 0; i < count; ++i)
            into.quadTo(ring.at(i), middle(ring.at(i), ring.at((i + 1) % count)));
        into.closeSubpath();
        return;
    }

    int start = 0;
    while (!corner.at(start))
        ++start;

    into.moveTo(ring.at(start));
    for (int step = 1; step <= count; ++step) {
        const int i = (start + step) % count;
        if (corner.at(i)) {
            into.lineTo(ring.at(i));
            continue;
        }
        const int next = (i + 1) % count;
        // Leaving a corner the curve starts at the corner itself, so the
        // first control point is the one that follows it.
        into.quadTo(ring.at(i), corner.at(next) ? ring.at(next) : middle(ring.at(i), ring.at(next)));
        if (corner.at(next))
            ++step;   // the corner has just been reached, do not draw it twice
    }
    into.closeSubpath();
}

}   // namespace

QPainterPath FloodFill::traced(const QPainterPath &path, qreal cornerDegrees)
{
    QPainterPath curved;
    curved.setFillRule(path.fillRule());

    const QList<QPolygonF> rings = path.toSubpathPolygons();
    for (const QPolygonF &ring : rings) {
        QPolygonF closed = ring;
        if (!closed.isEmpty() && closed.first() == closed.last())
            closed.removeLast();
        traceRing(closed, cornerDegrees, curved);
    }

    return curved.isEmpty() ? path : curved;
}

QPainterPath FloodFill::straightened(const QPainterPath &path, qreal tolerance)
{
    if (tolerance <= 0.0)
        return path;

    QPainterPath straight;
    straight.setFillRule(path.fillRule());

    const QList<QPolygonF> rings = path.toSubpathPolygons();
    for (const QPolygonF &ring : rings) {
        const QPolygonF simple = straightenRing(ring, tolerance);
        if (simple.size() < 3)
            continue;   // nothing left that could enclose an area
        straight.addPolygon(simple);
        straight.closeSubpath();
    }

    return straight.isEmpty() ? path : straight;
}

QPainterPath FloodFill::regionAt(const QImage &source, const QPoint &start, int tolerance, int grow)
{
    QPainterPath path;
    if (source.isNull() || !source.rect().contains(start))
        return path;

    const QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const int width = image.width();
    const int height = image.height();
    const QRgb target = image.pixel(start);

    QVector<bool> inside(width * height, false);

    // Scanline flood fill: each run of matching pixels is taken in one go and
    // only the rows above and below are put on the stack.
    QStack<QPoint> seeds;
    seeds.push(start);

    while (!seeds.isEmpty()) {
        const QPoint seed = seeds.pop();
        const int y = seed.y();
        if (y < 0 || y >= height)
            continue;

        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));

        int left = seed.x();
        while (left >= 0 && !inside.at(y * width + left) && matches(line[left], target, tolerance))
            --left;
        ++left;

        int right = seed.x();
        while (right < width && !inside.at(y * width + right)
               && matches(line[right], target, tolerance))
            ++right;
        --right;

        if (left > right)
            continue;

        for (int x = left; x <= right; ++x)
            inside[y * width + x] = true;

        for (int step = -1; step <= 1; step += 2) {
            const int nextY = y + step;
            if (nextY < 0 || nextY >= height)
                continue;

            const auto *nextLine = reinterpret_cast<const QRgb *>(image.constScanLine(nextY));
            bool inRun = false;
            for (int x = left; x <= right; ++x) {
                const bool usable = !inside.at(nextY * width + x)
                                    && matches(nextLine[x], target, tolerance);
                if (usable && !inRun) {
                    seeds.push(QPoint(x, nextY));
                    inRun = true;
                } else if (!usable) {
                    inRun = false;
                }
            }
        }
    }

    if (grow > 0)
        dilate(inside, width, height, grow);

    // A one bit mask turns into a QRegion, and that into a path. QBitmap
    // follows the mask convention, where the black entry of the colour table
    // is the one that makes up the region - with the table the other way round
    // the result would be the area around the fill instead of the fill itself.
    QImage mask(width, height, QImage::Format_MonoLSB);
    mask.setColorCount(2);
    mask.setColor(0, qRgb(255, 255, 255));
    mask.setColor(1, qRgb(0, 0, 0));
    mask.fill(0u);

    bool any = false;
    for (int y = 0; y < height; ++y) {
        uchar *line = mask.scanLine(y);
        for (int x = 0; x < width; ++x) {
            if (inside.at(y * width + x)) {
                line[x >> 3] |= uchar(1u << (x & 7));
                any = true;
            }
        }
    }
    if (!any)
        return path;

    path.addRegion(QRegion(QBitmap::fromImage(mask)));

    // The region arrives as a heap of adjacent rectangles. Merging them into
    // outlines keeps the project file small and the path quick to draw.
    return path.simplified();
}

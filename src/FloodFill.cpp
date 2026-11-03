#include "FloodFill.h"

#include <QBitmap>
#include <QImage>
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

}   // namespace

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

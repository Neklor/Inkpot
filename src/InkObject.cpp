#include "InkObject.h"

#include <QGraphicsItem>

void InkObject::eraseScenePath(const QPainterPath &scenePath)
{
    QGraphicsItem *item = asItem();
    if (!item)
        return;

    // Die Maske wächst mit jedem Radierer-Zug. Die Außenmaße des Objekts
    // ändern sich dabei nicht, ein update() genuegt also.
    m_erasedMask = m_erasedMask.united(item->mapFromScene(scenePath));
    item->update();
}

void InkObject::setErasedMask(const QPainterPath &mask)
{
    m_erasedMask = mask;
    if (QGraphicsItem *item = asItem())
        item->update();
}

QPainterPath InkObject::visibleClipPath(const QRectF &bounds) const
{
    QPainterPath whole;
    whole.addRect(bounds);
    if (m_erasedMask.isEmpty())
        return whole;
    return whole.subtracted(m_erasedMask);
}

QPainterPath InkObject::withoutErasedArea(const QPainterPath &path) const
{
    if (m_erasedMask.isEmpty())
        return path;
    return path.subtracted(m_erasedMask);
}

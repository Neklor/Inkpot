#include "InkObject.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QGraphicsItem>

QString InkPath::toBase64(const QPainterPath &path)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << path;
    return QString::fromLatin1(bytes.toBase64());
}

QPainterPath InkPath::fromBase64(const QString &encoded)
{
    QPainterPath path;
    QByteArray bytes = QByteArray::fromBase64(encoded.toLatin1());
    if (bytes.isEmpty())
        return path;
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    stream >> path;
    return path;
}

void InkObject::setPlacement(qreal angle, qreal scaleX, qreal scaleY)
{
    m_angle = angle;
    m_scaleX = qMax(qreal(0.01), scaleX);
    m_scaleY = qMax(qreal(0.01), scaleY);
    applyPlacement();
}

void InkObject::applyPlacement()
{
    QGraphicsItem *item = asItem();
    if (!item)
        return;

    // Read from the inside out: move the centre to the origin, scale along the
    // object's own axes, turn, move back.
    const QPointF centre = item->boundingRect().center();
    QTransform matrix;
    matrix.translate(centre.x(), centre.y());
    matrix.rotate(m_angle);
    matrix.scale(m_scaleX, m_scaleY);
    matrix.translate(-centre.x(), -centre.y());
    item->setTransform(matrix);
}

void InkObject::eraseScenePath(const QPainterPath &scenePath)
{
    QGraphicsItem *item = asItem();
    if (!item)
        return;

    // The mask grows with every stroke of the eraser. The outer dimensions of
    // the object do not change, so an update() is enough.
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

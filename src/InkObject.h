#pragma once

#include <QColor>
#include <QJsonObject>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QGraphicsItem;

// Shared interface of every object on the canvas.
//
// Anything that is drawn or pasted becomes an object in its own right: it also
// derives from a QGraphicsItem class and can therefore be selected, moved,
// scaled and deleted.
class InkObject
{
public:
    virtual ~InkObject() = default;

    // Identifier used in the project format, for example "stroke".
    virtual QString typeName() const = 0;

    // Name shown in the user interface.
    virtual QString displayName() const = 0;

    // Every object is a QGraphicsItem as well.
    virtual QGraphicsItem *asItem() = 0;

    // Type specific data only. Position, stacking order, scale and erased
    // areas are written and read by the scene for all objects alike.
    virtual void write(QJsonObject &json) const = 0;
    virtual bool read(const QJsonObject &json) = 0;

    // Colour of the object. Not every object has one - an image does not -
    // in which case the colour is invalid and setColor() reports false.
    virtual QColor color() const { return QColor(); }
    virtual bool setColor(const QColor &color) { Q_UNUSED(color); return false; }

    // Erasing inside an object: the area is masked out while the original
    // shape is kept, so it can be brought back later.
    void eraseScenePath(const QPainterPath &scenePath);
    bool hasErasedArea() const { return !m_erasedMask.isEmpty(); }
    QPainterPath erasedMask() const { return m_erasedMask; }
    void setErasedMask(const QPainterPath &mask);

protected:
    // The visible area within bounds, that is without the erased parts.
    QPainterPath visibleClipPath(const QRectF &bounds) const;

    // The shape minus the erased parts, used for hit testing.
    QPainterPath withoutErasedArea(const QPainterPath &path) const;

    QPainterPath m_erasedMask;   // in item coordinates
};

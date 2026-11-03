#pragma once

#include <QColor>
#include <QJsonObject>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QGraphicsItem;

// Paths are stored in the project file as base64 encoded QDataStream data.
// Both the erased areas and the area of a bucket fill use this.
namespace InkPath {
QString toBase64(const QPainterPath &path);
QPainterPath fromBase64(const QString &encoded);
}

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

    // Placement: rotation in degrees and scale along the object's own axes.
    //
    // Both are kept here rather than left to QGraphicsItem::setRotation(),
    // because Qt applies the item transform after its rotation. The scale
    // would then act on already rotated coordinates and distort the object
    // instead of letting it grow along its own axes. Building the matrix here
    // keeps the order right: scale first, then rotate, both about the centre.
    qreal rotationAngle() const { return m_angle; }
    qreal scaleX() const { return m_scaleX; }
    qreal scaleY() const { return m_scaleY; }
    void setPlacement(qreal angle, qreal scaleX, qreal scaleY);

    // Rebuilds the matrix, for instance after the content changed size.
    void applyPlacement();

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
    qreal m_angle = 0.0;
    qreal m_scaleX = 1.0;
    qreal m_scaleY = 1.0;
};

#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QRectF>

#include "InkObject.h"

// A geometric shape drawn with the shape tool.
//
// The outline is kept as a rectangle plus a kind, not as a fixed path, so the
// shape stays crisp at any size and the project file stays small.
class ShapeItem : public QGraphicsItem, public InkObject
{
public:
    enum class Kind {
        Ellipse,
        Rectangle,
        Triangle,
        Heart,
        Star
    };

    ShapeItem(Kind kind = Kind::Rectangle, const QColor &color = Qt::black,
              bool filled = true, qreal outlineWidth = 4.0);

    Kind kind() const { return m_kind; }
    bool isFilled() const { return m_filled; }

    // While drawing: the two corners the pointer spans.
    void setGeometry(const QPointF &origin, const QPointF &corner, bool keepSquare);
    QRectF geometry() const { return m_rect; }
    bool isTooSmall() const;

    // The outline of a kind inside a rectangle, used for drawing and for the
    // small pictures in the toolbar.
    static QPainterPath pathFor(Kind kind, const QRectF &rect);
    static QString nameFor(Kind kind);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString typeName() const override { return QStringLiteral("shape"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }

    QColor color() const override { return m_color; }
    bool setColor(const QColor &color) override;

    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;

private:
    Kind m_kind;
    QRectF m_rect;
    QColor m_color;
    bool m_filled;
    qreal m_outlineWidth;
};

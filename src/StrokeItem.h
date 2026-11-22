#pragma once

#include <QColor>
#include <QGraphicsPathItem>
#include <QPen>
#include <QVector>

#include "InkObject.h"

// A drawn stroke: freehand with the brush or straight with the line tool.
class StrokeItem : public QGraphicsPathItem, public InkObject
{
public:
    explicit StrokeItem(const QColor &color = Qt::black, qreal width = 4.0);

    // Drawing: beginAt() on press, extendTo() while dragging.
    void beginAt(const QPointF &scenePos);
    void extendTo(const QPointF &scenePos);

    // Line tool: drags a straight segment away from the starting point.
    void setLineEnd(const QPointF &scenePos);

    bool isEmpty() const { return m_points.isEmpty(); }
    qreal length() const;

    // A line drawn with the line tool has exactly two points. A brush stroke
    // has many, and giving it an angle and a length would mean nothing.
    bool isStraightLine() const { return m_points.size() == 2; }

    // The two ends as they lie on the canvas, rotation and scale included.
    QPointF lineStart() const;
    QPointF lineEnd() const;

    // Puts both ends where they are asked to be, in canvas coordinates, and
    // clears any rotation and scale: the two ends say everything there is to
    // say about where a straight line lies.
    void setLine(const QPointF &from, const QPointF &to);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QColor color() const override { return pen().color(); }
    bool setColor(const QColor &color) override;

    QString typeName() const override { return QStringLiteral("stroke"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }
    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;

private:
    void rebuildPath();
    qreal hitWidth() const;

    QVector<QPointF> m_points;   // in item coordinates
};

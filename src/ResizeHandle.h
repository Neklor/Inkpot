#pragma once

#include <QGraphicsItem>

class CanvasScene;

// A handle for changing a size.
//
// The same class serves two purposes: the canvas itself (role Canvas, right
// and bottom edge only) and scaling a selected object (role Object, all round).
// Handles ignore the view transformation and keep their size at any zoom
// level.
class ResizeHandle : public QGraphicsItem
{
public:
    enum Position {
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left
    };

    enum class Role {
        Canvas,
        Object,
        Rotate
    };

    ResizeHandle(Position position, Role role, CanvasScene *canvas);

    Position handlePosition() const { return m_position; }
    Role role() const { return m_role; }

    // Centre of the handle for a given rectangle.
    static QPointF anchorPoint(Position position, const QRectF &rect);

    // The handle on the opposite side; its point stays put while dragging.
    static Position opposite(Position position);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Position m_position;
    Role m_role;
    CanvasScene *m_canvas;
};

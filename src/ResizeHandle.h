#pragma once

#include <QGraphicsItem>

class CanvasScene;

// Anfasspunkt zum Verändern einer Größe.
//
// Dieselbe Klasse bedient zwei Aufgaben: die Arbeitsfläche selbst (Rolle
// Canvas, nur rechts und unten) und das Skalieren eines ausgewählten Objekts
// (Rolle Object, rundherum). Die Griffe ignorieren die Ansichts-Transformation
// und bleiben bei jedem Zoomfaktor gleich gross.
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
        Object
    };

    ResizeHandle(Position position, Role role, CanvasScene *canvas);

    Position handlePosition() const { return m_position; }
    Role role() const { return m_role; }

    // Mittelpunkt des Griffs für ein gegebenes Rechteck.
    static QPointF anchorPoint(Position position, const QRectF &rect);

    // Der Griff auf der anderen Seite; sein Punkt bleibt beim Ziehen stehen.
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

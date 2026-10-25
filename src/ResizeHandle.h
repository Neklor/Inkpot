#pragma once

#include <QGraphicsItem>

class CanvasScene;

// Anfasspunkt am Rand der Arbeitsflaeche.
//
// Die Griffe ignorieren die Ansichts-Transformation, bleiben also bei jedem
// Zoomfaktor gleich gross.
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

    ResizeHandle(Position position, CanvasScene *canvas);

    Position handlePosition() const { return m_position; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Position m_position;
    CanvasScene *m_canvas;
};

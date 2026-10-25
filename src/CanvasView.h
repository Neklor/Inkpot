#pragma once

#include <QColor>
#include <QGraphicsView>
#include <QPoint>
#include <QPointF>

class CanvasScene;
class StrokeItem;
class TextItem;

// Ansicht der Arbeitsfläche samt Werkzeugbedienung.
class CanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    enum class Tool {
        Select,
        Brush,
        Line,
        Eraser,
        Text
    };

    explicit CanvasView(CanvasScene *scene, QWidget *parent = nullptr);

    Tool tool() const { return m_tool; }
    void setTool(Tool tool);

    QColor toolColor() const { return m_toolColor; }
    void setToolColor(const QColor &color);

    // Dient je nach Werkzeug als Strichstärke, Radierer-Durchmesser oder
    // Schriftgröße.
    qreal toolSize() const { return m_toolSize; }
    void setToolSize(qreal size);

    double zoomFactor() const;
    void setZoomFactor(double factor);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitPageInView();

signals:
    void zoomChanged(double factor);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void applyCursor();
    void eraseFrom(const QPointF &fromScene, const QPointF &toScene);
    void placeTextAt(const QPointF &scenePos);

    CanvasScene *m_canvas;
    Tool m_tool = Tool::Select;
    QColor m_toolColor = Qt::black;
    qreal m_toolSize = 4.0;

    StrokeItem *m_activeStroke = nullptr;
    bool m_erasing = false;
    QPointF m_lastErasePos;

    bool m_panning = false;
    QPoint m_lastPanPos;
    QPoint m_pressPos;
    bool m_dragMayMoveObjects = false;
};

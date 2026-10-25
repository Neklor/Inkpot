#pragma once

#include <QColor>
#include <QGraphicsView>
#include <QPoint>

class CanvasScene;
class StrokeItem;

// Ansicht der Arbeitsflaeche samt Werkzeugbedienung.
class CanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    enum class Tool {
        Select,
        Brush
    };

    explicit CanvasView(CanvasScene *scene, QWidget *parent = nullptr);

    Tool tool() const { return m_tool; }
    void setTool(Tool tool);

    QColor brushColor() const { return m_brushColor; }
    void setBrushColor(const QColor &color);

    qreal brushWidth() const { return m_brushWidth; }
    void setBrushWidth(qreal width);

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

    CanvasScene *m_canvas;
    Tool m_tool = Tool::Select;
    QColor m_brushColor = Qt::black;
    qreal m_brushWidth = 4.0;
    StrokeItem *m_activeStroke = nullptr;
    bool m_panning = false;
    QPoint m_lastPanPos;
    QPoint m_pressPos;
    bool m_dragMayMoveObjects = false;
};

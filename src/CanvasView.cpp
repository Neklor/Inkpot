#include "CanvasView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include "CanvasScene.h"
#include "StrokeItem.h"

namespace {
constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 32.0;
constexpr double kZoomStep = 1.25;
}

CanvasView::CanvasView(CanvasScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
    , m_canvas(scene)
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setBackgroundBrush(QColor(0x1f, 0x20, 0x24));
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    applyCursor();
}

void CanvasView::setTool(Tool tool)
{
    if (m_tool == tool)
        return;
    m_tool = tool;
    // Beim Malen stoert die Gummiband-Auswahl.
    setDragMode(m_tool == Tool::Select ? QGraphicsView::RubberBandDrag : QGraphicsView::NoDrag);
    applyCursor();
}

void CanvasView::applyCursor()
{
    if (m_panning)
        viewport()->setCursor(Qt::ClosedHandCursor);
    else
        viewport()->setCursor(m_tool == Tool::Brush ? Qt::CrossCursor : Qt::ArrowCursor);
}

void CanvasView::setBrushColor(const QColor &color)
{
    if (color.isValid())
        m_brushColor = color;
}

void CanvasView::setBrushWidth(qreal width)
{
    m_brushWidth = qBound(0.5, width, 200.0);
}

double CanvasView::zoomFactor() const
{
    return transform().m11();
}

void CanvasView::setZoomFactor(double factor)
{
    const double clamped = qBound(kMinZoom, factor, kMaxZoom);
    QTransform transform;
    transform.scale(clamped, clamped);
    setTransform(transform);
    emit zoomChanged(clamped);
}

void CanvasView::zoomIn()
{
    setZoomFactor(zoomFactor() * kZoomStep);
}

void CanvasView::zoomOut()
{
    setZoomFactor(zoomFactor() / kZoomStep);
}

void CanvasView::resetZoom()
{
    setZoomFactor(1.0);
}

void CanvasView::fitPageInView()
{
    const QRectF page = m_canvas->pageRect().adjusted(-24.0, -24.0, 24.0, 24.0);
    fitInView(page, Qt::KeepAspectRatio);
    emit zoomChanged(zoomFactor());
}

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        // Mittlere Maustaste verschiebt die Ansicht.
        m_panning = true;
        m_lastPanPos = event->pos();
        applyCursor();
        event->accept();
        return;
    }

    if (m_tool == Tool::Brush && event->button() == Qt::LeftButton) {
        m_activeStroke = new StrokeItem(m_brushColor, m_brushWidth);
        m_activeStroke->beginAt(mapToScene(event->pos()));
        m_canvas->addObject(m_activeStroke);
        event->accept();
        return;
    }

    m_pressPos = event->pos();
    m_dragMayMoveObjects = event->button() == Qt::LeftButton
                           && itemAt(event->pos()) != nullptr;

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    if (m_activeStroke) {
        m_activeStroke->extendTo(mapToScene(event->pos()));
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_panning && event->button() == Qt::MiddleButton) {
        m_panning = false;
        applyCursor();
        event->accept();
        return;
    }

    if (m_activeStroke && event->button() == Qt::LeftButton) {
        m_activeStroke->extendTo(mapToScene(event->pos()));
        m_activeStroke->setSelected(false);
        m_activeStroke = nullptr;
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);

    // Verschobene Objekte sind eine Aenderung am Dokument.
    if (m_dragMayMoveObjects
        && (event->pos() - m_pressPos).manhattanLength() > 2
        && !m_canvas->selectedItems().isEmpty()) {
        m_canvas->notifyEdited();
    }
    m_dragMayMoveObjects = false;
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int steps = event->angleDelta().y();
        if (steps > 0)
            zoomIn();
        else if (steps < 0)
            zoomOut();
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

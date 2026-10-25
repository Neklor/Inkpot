#include "CanvasView.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QScrollBar>
#include <QWheelEvent>

#include "CanvasScene.h"
#include "StrokeItem.h"
#include "TextItem.h"

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
    setResizeAnchor(QGraphicsView::NoAnchor);

    // Die Arbeitsfläche liegt fest in der linken oberen Ecke.
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Wichtig: kein eigener Hintergrundpinsel auf der Ansicht. Sobald einer
    // gesetzt ist, ruft Qt CanvasScene::drawBackground() nicht mehr auf und
    // die weisse Arbeitsfläche bliebe unsichtbar.
    applyCursor();
}

void CanvasView::setTool(Tool tool)
{
    if (m_tool == tool)
        return;
    m_tool = tool;
    // Nur das Auswahlwerkzeug zieht ein Gummiband auf.
    setDragMode(m_tool == Tool::Select ? QGraphicsView::RubberBandDrag : QGraphicsView::NoDrag);
    applyCursor();
}

void CanvasView::applyCursor()
{
    if (m_panning) {
        viewport()->setCursor(Qt::ClosedHandCursor);
        return;
    }

    switch (m_tool) {
    case Tool::Select:
        viewport()->setCursor(Qt::ArrowCursor);
        break;
    case Tool::Text:
        viewport()->setCursor(Qt::IBeamCursor);
        break;
    default:
        viewport()->setCursor(Qt::CrossCursor);
        break;
    }
}

void CanvasView::setToolColor(const QColor &color)
{
    if (color.isValid())
        m_toolColor = color;
}

void CanvasView::setToolSize(qreal size)
{
    m_toolSize = qBound(0.5, size, 200.0);
}

void CanvasView::setFontPointSize(qreal pointSize)
{
    m_fontPointSize = qBound(1.0, pointSize, 2000.0);
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
    const QRectF page = m_canvas->pageRect().adjusted(0.0, 0.0, 24.0, 24.0);
    fitInView(page, Qt::KeepAspectRatio);
    emit zoomChanged(zoomFactor());
}

void CanvasView::eraseFrom(const QPointF &fromScene, const QPointF &toScene)
{
    // Der Radierer wirkt als runde Spur zwischen den beiden Punkten.
    QPainterPath track;
    track.moveTo(fromScene);
    if (fromScene == toScene)
        track.lineTo(toScene + QPointF(0.01, 0.0));
    else
        track.lineTo(toScene);

    QPainterPathStroker stroker;
    stroker.setWidth(qMax<qreal>(m_toolSize, 1.0));
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    m_canvas->eraseWithPath(stroker.createStroke(track));
}

void CanvasView::placeTextAt(const QPointF &scenePos)
{
    // Auf vorhandenem Text weiterschreiben statt ein neues Objekt anzulegen.
    if (auto *existing = dynamic_cast<TextItem *>(itemAt(mapFromScene(scenePos)))) {
        existing->beginEditing();
        return;
    }

    auto *item = new TextItem;
    QFont font = item->font();
    font.setPointSizeF(m_fontPointSize);
    item->applyFont(font);
    item->setDefaultTextColor(m_toolColor);
    item->setPos(scenePos);

    m_canvas->addObject(item);
    item->beginEditing();
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

    if (event->button() == Qt::LeftButton) {
        const QPointF scenePos = mapToScene(event->pos());

        switch (m_tool) {
        case Tool::Brush:
            m_activeStroke = new StrokeItem(m_toolColor, m_toolSize);
            m_activeStroke->beginAt(scenePos);
            m_canvas->addObject(m_activeStroke);
            event->accept();
            return;

        case Tool::Line:
            m_activeStroke = new StrokeItem(m_toolColor, m_toolSize);
            m_activeStroke->beginAt(scenePos);
            m_activeStroke->setLineEnd(scenePos);
            m_canvas->addObject(m_activeStroke);
            event->accept();
            return;

        case Tool::Eraser:
            m_erasing = true;
            m_lastErasePos = scenePos;
            eraseFrom(scenePos, scenePos);
            event->accept();
            return;

        case Tool::Text:
            placeTextAt(scenePos);
            event->accept();
            return;

        case Tool::Select:
            break;
        }
    }

    m_pressPos = event->pos();
    m_dragMayMoveObjects = event->button() == Qt::LeftButton && itemAt(event->pos()) != nullptr;

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
        const QPointF scenePos = mapToScene(event->pos());
        if (m_tool == Tool::Line)
            m_activeStroke->setLineEnd(scenePos);
        else
            m_activeStroke->extendTo(scenePos);
        event->accept();
        return;
    }

    if (m_erasing) {
        const QPointF scenePos = mapToScene(event->pos());
        eraseFrom(m_lastErasePos, scenePos);
        m_lastErasePos = scenePos;
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
        const QPointF scenePos = mapToScene(event->pos());
        if (m_tool == Tool::Line) {
            m_activeStroke->setLineEnd(scenePos);
            // Ein versehentlicher Klick ohne Ziehen hinterlaesst keine Linie.
            if (m_activeStroke->length() < 2.0)
                m_canvas->removeObjectLater(m_activeStroke);
        } else {
            m_activeStroke->extendTo(scenePos);
        }
        m_activeStroke->setSelected(false);
        m_activeStroke = nullptr;
        event->accept();
        return;
    }

    if (m_erasing && event->button() == Qt::LeftButton) {
        m_erasing = false;
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);

    // Verschobene Objekte sind eine Änderung am Dokument.
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

#include "ResizeHandle.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "CanvasScene.h"

namespace {
constexpr qreal kHandleSize = 9.0;   // Bildschirmpixel, zoomunabhaengig
}

ResizeHandle::ResizeHandle(Position position, CanvasScene *canvas)
    : m_position(position)
    , m_canvas(canvas)
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setZValue(1.0e6);                       // immer ueber allen Objekten
    setAcceptedMouseButtons(Qt::LeftButton);

    switch (m_position) {
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    }

    setToolTip(QObject::tr("Ziehen, um die Arbeitsflaeche zu veraendern"));
}

QRectF ResizeHandle::boundingRect() const
{
    const qreal half = kHandleSize / 2.0 + 1.0;
    return QRectF(-half, -half, half * 2.0, half * 2.0);
}

void ResizeHandle::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    const qreal half = kHandleSize / 2.0;
    const QRectF square(-half, -half, kHandleSize, kHandleSize);

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(QColor(0x1c, 0x1d, 0x21), 1.0));
    painter->setBrush(QColor(0xf2, 0xf3, 0xf5));
    painter->drawRect(square);
}

void ResizeHandle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_canvas->beginHandleResize();
    event->accept();
}

void ResizeHandle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    m_canvas->resizeFromHandle(m_position, event->scenePos());
    event->accept();
}

void ResizeHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_canvas->endHandleResize();
    event->accept();
}

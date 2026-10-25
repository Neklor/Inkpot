#include "ResizeHandle.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "CanvasScene.h"

namespace {
constexpr qreal kHandleSize = 9.0;   // Bildschirmpixel, zoomunabhaengig
}

ResizeHandle::ResizeHandle(Position position, Role role, CanvasScene *canvas)
    : m_position(position)
    , m_role(role)
    , m_canvas(canvas)
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    // Objektgriffe liegen über den Objekten, Flächengriffe darüber.
    setZValue(m_role == Role::Canvas ? 1.0e6 : 9.0e5);
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

    setToolTip(m_role == Role::Canvas
                   ? QObject::tr("Ziehen, um die Arbeitsfläche zu verändern")
                   : QObject::tr("Ziehen, um das Objekt zu skalieren"));
}

QPointF ResizeHandle::anchorPoint(Position position, const QRectF &rect)
{
    switch (position) {
    case TopLeft:     return rect.topLeft();
    case Top:         return QPointF(rect.center().x(), rect.top());
    case TopRight:    return rect.topRight();
    case Right:       return QPointF(rect.right(), rect.center().y());
    case BottomRight: return rect.bottomRight();
    case Bottom:      return QPointF(rect.center().x(), rect.bottom());
    case BottomLeft:  return rect.bottomLeft();
    case Left:        return QPointF(rect.left(), rect.center().y());
    }
    return rect.center();
}

ResizeHandle::Position ResizeHandle::opposite(Position position)
{
    switch (position) {
    case TopLeft:     return BottomRight;
    case Top:         return Bottom;
    case TopRight:    return BottomLeft;
    case Right:       return Left;
    case BottomRight: return TopLeft;
    case Bottom:      return Top;
    case BottomLeft:  return TopRight;
    case Left:        return Right;
    }
    return TopLeft;
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
    if (m_role == Role::Canvas) {
        painter->setPen(QPen(QColor(0x1c, 0x1d, 0x21), 1.0));
        painter->setBrush(QColor(0xf2, 0xf3, 0xf5));
        painter->drawRect(square);
    } else {
        // Objektgriffe heben sich farblich von den Flächengriffen ab.
        painter->setPen(QPen(QColor(0x10, 0x3a, 0x6b), 1.0));
        painter->setBrush(QColor(0x4d, 0xa3, 0xff));
        painter->drawRect(square);
    }
}

void ResizeHandle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_role == Role::Canvas)
        m_canvas->beginCanvasResize();
    else
        m_canvas->beginObjectScale();
    event->accept();
}

void ResizeHandle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_role == Role::Canvas)
        m_canvas->resizeCanvasFromHandle(m_position, event->scenePos());
    else
        m_canvas->scaleObjectFromHandle(m_position, event->scenePos());
    event->accept();
}

void ResizeHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_role == Role::Canvas)
        m_canvas->endCanvasResize();
    else
        m_canvas->endObjectScale();
    event->accept();
}

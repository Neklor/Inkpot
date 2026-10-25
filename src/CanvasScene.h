#pragma once

#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QPainterPath>
#include <QRectF>
#include <QSize>
#include <QTransform>

#include "ResizeHandle.h"

// Die Szene enthaelt die Arbeitsfläche (die "Seite") und alle Objekte darauf.
//
// Die Arbeitsfläche liegt fest in der linken oberen Ecke bei (0, 0) und wächst
// nach rechts und unten. Veraendert wird sie über die drei Anfasspunkte an der
// rechten und unteren Kante oder über die Felder in der Fusszeile.
class CanvasScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);
    ~CanvasScene() override;

    QRectF pageRect() const { return QRectF(QPointF(0.0, 0.0), QSizeF(m_pageSize)); }
    QSize pageSize() const { return m_pageSize; }

    void setPageSize(const QSize &size);
    void setPageWidth(int width);
    void setPageHeight(int height);

    bool aspectRatioLocked() const { return m_aspectLocked; }
    void setAspectRatioLocked(bool locked);
    double lockedRatio() const { return m_lockedRatio; }

    // Ziehen an einem Anfasspunkt der Arbeitsfläche.
    void beginCanvasResize();
    void resizeCanvasFromHandle(ResizeHandle::Position position, const QPointF &scenePos);
    void endCanvasResize();

    // Ziehen an einem Anfasspunkt eines ausgewählten Objekts.
    void beginObjectScale();
    void scaleObjectFromHandle(ResizeHandle::Position position, const QPointF &scenePos);
    void endObjectScale();

    // Objektverwaltung.
    void addObject(QGraphicsItem *item);
    QList<QGraphicsItem *> objects() const;
    int objectCount() const { return objects().size(); }
    void removeSelectedObjects();
    void removeObjectLater(QGraphicsItem *item);
    void clearObjects();
    void selectAllObjects();
    QGraphicsItem *singleSelectedObject() const;

    // Radieren innerhalb der Objekte.
    void eraseWithPath(const QPainterPath &scenePath);

    // Von aussen aufrufbar, wenn Objekte z. B. verschoben wurden.
    void notifyEdited() { emit edited(); }

    // Ausgabe als Bild. Anfasspunkte und Auswahlrahmen werden ausgeblendet.
    QImage renderRegion(const QRectF &source,
                        const QList<QGraphicsItem *> &onlyItems = QList<QGraphicsItem *>(),
                        const QColor &background = Qt::white,
                        qreal scale = 1.0);
    QImage renderPage(const QColor &background = Qt::white, qreal scale = 1.0);
    QImage renderSelection();

    // Projektformat (*.inkpot).
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json, QString *error = nullptr);

signals:
    void pageRectChanged();
    void objectsChanged();
    void edited();          // es gibt ungespeicherte Änderungen

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void createHandles();
    void updateCanvasHandles();
    void updateObjectHandles();
    void setHandlesVisible(bool visible);
    void updateSceneRect();
    void prepareObject(QGraphicsItem *item);

    QSize m_pageSize = QSize(1280, 720);
    bool m_aspectLocked = false;
    double m_lockedRatio = 16.0 / 9.0;
    bool m_renderingToImage = false;
    bool m_updatingHandles = false;
    qreal m_topZ = 0.0;

    QList<ResizeHandle *> m_canvasHandles;
    QList<ResizeHandle *> m_objectHandles;

    // Ausgangswerte während des Skalierens eines Objekts.
    QGraphicsItem *m_scaleTarget = nullptr;
    QRectF m_scaleStartRect;
    QTransform m_scaleStartTransform;
};

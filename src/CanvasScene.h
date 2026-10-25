#pragma once

#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QRectF>
#include <QSize>

#include "ResizeHandle.h"

// Die Szene enthaelt die Arbeitsflaeche (die "Seite") und alle Objekte darauf.
//
// Die Arbeitsflaeche ist ein Rechteck in Szenenkoordinaten. Sie kann ueber die
// acht Anfasspunkte oder ueber die Fusszeile veraendert werden; die Objekte
// behalten dabei ihre Position.
class CanvasScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);

    QRectF pageRect() const { return m_pageRect; }
    QSize pageSize() const { return m_pageRect.size().toSize(); }

    void setPageRect(const QRectF &rect);
    void setPageSize(const QSize &size);
    void setPageWidth(int width);
    void setPageHeight(int height);

    bool aspectRatioLocked() const { return m_aspectLocked; }
    void setAspectRatioLocked(bool locked);
    double lockedRatio() const { return m_lockedRatio; }

    // Ziehen an einem Anfasspunkt.
    void beginHandleResize();
    void resizeFromHandle(ResizeHandle::Position position, const QPointF &scenePos);
    void endHandleResize();

    // Objektverwaltung.
    void addObject(QGraphicsItem *item);
    QList<QGraphicsItem *> objects() const;
    int objectCount() const { return objects().size(); }
    void removeSelectedObjects();
    void clearObjects();
    void selectAllObjects();

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
    void edited();          // es gibt ungespeicherte Aenderungen

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void createHandles();
    void updateHandlePositions();
    void setHandlesVisible(bool visible);
    void updateSceneRect();
    void prepareObject(QGraphicsItem *item);

    QRectF m_pageRect;
    bool m_aspectLocked = false;
    double m_lockedRatio = 16.0 / 9.0;
    bool m_renderingToImage = false;
    qreal m_topZ = 0.0;
    QList<ResizeHandle *> m_handles;
};

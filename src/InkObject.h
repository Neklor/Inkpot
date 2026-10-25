#pragma once

#include <QJsonObject>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QGraphicsItem;

// Gemeinsame Schnittstelle aller Objekte auf der Arbeitsfläche.
//
// Alles, was gemalt oder eingefügt wird, ist ein eigenständiges Objekt: es
// erbt zusaetzlich von einer QGraphicsItem-Klasse und ist damit auswaehlbar,
// verschiebbar, skalierbar und loeschbar.
class InkObject
{
public:
    virtual ~InkObject() = default;

    // Bezeichner im Projektformat, z. B. "stroke".
    virtual QString typeName() const = 0;

    // Für die Anzeige in der Oberflaeche.
    virtual QString displayName() const = 0;

    // Jedes Objekt ist zugleich ein QGraphicsItem.
    virtual QGraphicsItem *asItem() = 0;

    // Nur die typeigenen Daten. Position, Ebene, Skalierung und Radierspuren
    // schreibt und liest die Szene für alle Objekte einheitlich.
    virtual void write(QJsonObject &json) const = 0;
    virtual bool read(const QJsonObject &json) = 0;

    // Radieren innerhalb eines Objekts: der Bereich wird maskiert, die
    // ursprüngliche Form bleibt erhalten und kann später wieder
    // freigegeben werden.
    void eraseScenePath(const QPainterPath &scenePath);
    bool hasErasedArea() const { return !m_erasedMask.isEmpty(); }
    QPainterPath erasedMask() const { return m_erasedMask; }
    void setErasedMask(const QPainterPath &mask);

protected:
    // Sichtbarer Bereich innerhalb von bounds, also ohne die Radierspuren.
    QPainterPath visibleClipPath(const QRectF &bounds) const;

    // Form abzueglich der Radierspuren, für die Treffer-Prüfung.
    QPainterPath withoutErasedArea(const QPainterPath &path) const;

    QPainterPath m_erasedMask;   // in Item-Koordinaten
};

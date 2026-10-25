#pragma once

#include <QJsonObject>
#include <QPointF>
#include <QString>

// Gemeinsame Schnittstelle aller Objekte auf der Arbeitsflaeche.
//
// Alles, was gemalt oder eingefuegt wird, ist ein eigenstaendiges Objekt: es
// erbt zusaetzlich von einer QGraphicsItem-Klasse und ist damit auswaehlbar,
// verschiebbar und loeschbar. Ueber diese Schnittstelle laeuft das Speichern
// und Laden im Projektformat.
class InkObject
{
public:
    virtual ~InkObject() = default;

    // Bezeichner im Projektformat, z. B. "stroke".
    virtual QString typeName() const = 0;

    // Fuer die Anzeige in der Oberflaeche.
    virtual QString displayName() const = 0;

    // Positionen werden relativ zur linken oberen Ecke der Arbeitsflaeche
    // gespeichert, damit verschobene Arbeitsflaechen nichts veraendern.
    virtual void write(QJsonObject &json, const QPointF &origin) const = 0;
    virtual bool read(const QJsonObject &json) = 0;
};

#pragma once

#include <QFont>
#include <QGraphicsTextItem>

#include "InkObject.h"

// Beschriftung auf der Arbeitsfläche.
//
// Im Bearbeitungsmodus nimmt das Objekt Tastatureingaben an; danach verhaelt
// es sich wie jedes andere Objekt und laesst sich auswählen, verschieben und
// skalieren.
class TextItem : public QGraphicsTextItem, public InkObject
{
    Q_OBJECT

public:
    explicit TextItem(const QString &text = QString());

    // Setzt die Schrift und sorgt dabei für saubere Graustufen-Glättung.
    void applyFont(const QFont &font);

    // Schriftgröße in Punkt. Über die Anfasspunkte wird genau dieser Wert
    // verändert, damit Text nicht verzerrt, sondern wirklich größer wird.
    qreal fontPointSize() const;
    void setFontPointSize(qreal pointSize);

    QColor color() const override { return defaultTextColor(); }
    bool setColor(const QColor &color) override;

    void beginEditing();
    void endEditing();
    bool isEditing() const;

    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString typeName() const override { return QStringLiteral("text"); }
    QString displayName() const override;
    QGraphicsItem *asItem() override { return this; }
    void write(QJsonObject &json) const override;
    bool read(const QJsonObject &json) override;

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};

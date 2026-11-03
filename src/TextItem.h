#pragma once

#include <QFont>
#include <QGraphicsTextItem>

#include "InkObject.h"

// A caption on the canvas.
//
// While being edited the object accepts keyboard input; afterwards it behaves
// like any other object and can be selected, moved and scaled.
class TextItem : public QGraphicsTextItem, public InkObject
{
    Q_OBJECT

public:
    explicit TextItem(const QString &text = QString());

    // Sets the font and makes sure it is antialiased in grayscale.
    void applyFont(const QFont &font);

    // Font size in points. The handles change exactly this value, so that
    // text really grows instead of being distorted.
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

private:
    QString m_textBeforeEditing;   // to tell the history what changed
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};

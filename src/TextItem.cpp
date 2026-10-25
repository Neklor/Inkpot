#include "TextItem.h"

#include <QCoreApplication>
#include <QFocusEvent>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextCursor>

#include "CanvasScene.h"

TextItem::TextItem(const QString &text)
    : QGraphicsTextItem(text)
{
    setTextInteractionFlags(Qt::NoTextInteraction);
    applyFont(font());
}

void TextItem::applyFont(const QFont &newFont)
{
    // Subpixel-Glättung erzeugt farbige Ränder an den Buchstaben. Auf einer
    // Malfläche, die auch exportiert und gedruckt wird, sollen Buchstaben rein
    // in ihrer eigenen Farbe erscheinen.
    QFont adjusted = newFont;
    adjusted.setStyleStrategy(QFont::StyleStrategy(adjusted.styleStrategy()
                                                   | QFont::NoSubpixelAntialias));
    setFont(adjusted);
}

QString TextItem::displayName() const
{
    return QCoreApplication::translate("TextItem", "Text");
}

bool TextItem::isEditing() const
{
    return textInteractionFlags() != Qt::NoTextInteraction;
}

void TextItem::beginEditing()
{
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus(Qt::MouseFocusReason);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void TextItem::endEditing()
{
    if (!isEditing())
        return;

    setTextInteractionFlags(Qt::NoTextInteraction);

    // Markierung im Text aufheben, sonst bleibt sie sichtbar.
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

QPainterPath TextItem::shape() const
{
    return withoutErasedArea(QGraphicsTextItem::shape());
}

void TextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!hasErasedArea()) {
        QGraphicsTextItem::paint(painter, option, widget);
        return;
    }

    painter->save();
    painter->setClipPath(visibleClipPath(boundingRect()), Qt::IntersectClip);
    QGraphicsTextItem::paint(painter, option, widget);
    painter->restore();
}

void TextItem::focusOutEvent(QFocusEvent *event)
{
    endEditing();
    QGraphicsTextItem::focusOutEvent(event);

    // Leere Beschriftungen sind nur Überbleibsel eines Fehlklicks.
    if (toPlainText().trimmed().isEmpty()) {
        if (auto *canvas = qobject_cast<CanvasScene *>(scene()))
            canvas->removeObjectLater(this);
    }
}

void TextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    beginEditing();
    QGraphicsTextItem::mouseDoubleClickEvent(event);
}

void TextItem::write(QJsonObject &json) const
{
    json[QStringLiteral("text")] = toPlainText();
    json[QStringLiteral("font")] = font().toString();
    json[QStringLiteral("color")] = defaultTextColor().name(QColor::HexArgb);
}

bool TextItem::read(const QJsonObject &json)
{
    setPlainText(json.value(QStringLiteral("text")).toString());

    QFont restored;
    if (restored.fromString(json.value(QStringLiteral("font")).toString()))
        applyFont(restored);

    const QColor color(json.value(QStringLiteral("color")).toString(QStringLiteral("#ff000000")));
    if (color.isValid())
        setDefaultTextColor(color);

    return true;
}

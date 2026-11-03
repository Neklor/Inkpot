#include "TextItem.h"

#include <QCoreApplication>
#include <QFocusEvent>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

#include "CanvasScene.h"

TextItem::TextItem(const QString &text)
    : QGraphicsTextItem(text)
{
    setTextInteractionFlags(Qt::NoTextInteraction);
    applyFont(font());
}

void TextItem::applyFont(const QFont &newFont)
{
    // Subpixel antialiasing puts coloured fringes around the glyphs. On a
    // canvas that is also exported and printed, letters should appear purely
    // in their own colour.
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

qreal TextItem::fontPointSize() const
{
    return font().pointSizeF();
}

void TextItem::setFontPointSize(qreal pointSize)
{
    const qreal clamped = qBound(1.0, pointSize, 2000.0);

    QFont updated = font();
    if (qFuzzyCompare(updated.pointSizeF(), clamped))
        return;
    updated.setPointSizeF(clamped);
    applyFont(updated);

    // Text that has already been typed carries its own character formats;
    // the document font alone would not reach it.
    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setFontPointSize(clamped);
    cursor.mergeCharFormat(format);
}

bool TextItem::setColor(const QColor &color)
{
    if (!color.isValid())
        return false;

    setDefaultTextColor(color);

    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setForeground(color);
    cursor.mergeCharFormat(format);

    update();
    return true;
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

    // Drop the text selection, otherwise it stays visible.
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

    // An empty caption is just the leftover of a misplaced click.
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

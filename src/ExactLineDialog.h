#pragma once

#include <QDialog>
#include <QPointF>

class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;

// Giving a line a direction and a length as numbers instead of dragging it.
//
// The angle is counted the way it is on paper: zero points to the right and
// the count runs anticlockwise, so ninety degrees points up. On screen the y
// axis runs the other way, which QLineF already takes care of.
class ExactLineDialog : public QDialog
{
    Q_OBJECT

public:
    // start and the other two describe the line the dialog opens with. Pass
    // editing when an existing line is being changed rather than a new one
    // drawn: only the wording differs.
    ExactLineDialog(const QPointF &start, qreal angleDegrees, qreal length,
                    bool editing, QWidget *parent = nullptr);

    QPointF start() const;
    qreal angleDegrees() const;
    qreal length() const;

    // Where the line ends up, which is what the dialog shows while typing.
    QPointF end() const;

private:
    void updatePreview();

    QDoubleSpinBox *m_x;
    QDoubleSpinBox *m_y;
    QDoubleSpinBox *m_angle;
    QDoubleSpinBox *m_length;
    QLabel *m_preview;
    QDialogButtonBox *m_buttons;
};

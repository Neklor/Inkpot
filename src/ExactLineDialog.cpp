#include "ExactLineDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineF>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QDoubleSpinBox *numberField(double smallest, double largest, const QString &suffix)
{
    auto *box = new QDoubleSpinBox;
    box->setRange(smallest, largest);
    box->setDecimals(1);
    box->setSingleStep(1.0);
    if (!suffix.isEmpty())
        box->setSuffix(suffix);
    return box;
}

}   // namespace

ExactLineDialog::ExactLineDialog(const QPointF &start, qreal angleDegrees, qreal length,
                                 bool editing, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(editing ? tr("Change the Line") : tr("Exact Line"));

    m_x = numberField(-100000.0, 100000.0, tr(" px"));
    m_y = numberField(-100000.0, 100000.0, tr(" px"));

    // A full turn and a little over, so a line can be given 370 degrees
    // without having to work out that this is the same as 10.
    m_angle = numberField(-360.0, 720.0, tr(" °"));
    m_angle->setWrapping(true);

    m_length = numberField(0.1, 100000.0, tr(" px"));

    m_x->setValue(start.x());
    m_y->setValue(start.y());
    m_angle->setValue(angleDegrees);
    m_length->setValue(length);

    auto *coordinates = new QHBoxLayout;
    coordinates->setContentsMargins(0, 0, 0, 0);
    coordinates->addWidget(new QLabel(tr("X")));
    coordinates->addWidget(m_x);
    coordinates->addSpacing(8);
    coordinates->addWidget(new QLabel(tr("Y")));
    coordinates->addWidget(m_y);

    auto *form = new QFormLayout;
    form->addRow(tr("Starts at"), coordinates);
    form->addRow(tr("Direction"), m_angle);
    form->addRow(tr("Length"), m_length);

    m_preview = new QLabel;
    m_preview->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *hint = new QLabel(tr("Zero degrees points to the right, and the count runs "
                               "anticlockwise, so ninety degrees points up."));
    hint->setWordWrap(true);
    QPalette faded = hint->palette();
    faded.setColor(QPalette::WindowText, palette().color(QPalette::Dark));
    hint->setPalette(faded);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttons->button(QDialogButtonBox::Ok)
        ->setText(editing ? tr("Change") : tr("Draw"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *column = new QVBoxLayout(this);
    column->addLayout(form);
    column->addWidget(m_preview);
    column->addSpacing(4);
    column->addWidget(hint);
    column->addStretch();
    column->addWidget(m_buttons);

    for (QDoubleSpinBox *box : { m_x, m_y, m_angle, m_length }) {
        connect(box, &QDoubleSpinBox::valueChanged, this, &ExactLineDialog::updatePreview);
    }
    updatePreview();
}

QPointF ExactLineDialog::start() const
{
    return QPointF(m_x->value(), m_y->value());
}

qreal ExactLineDialog::angleDegrees() const
{
    return m_angle->value();
}

qreal ExactLineDialog::length() const
{
    return m_length->value();
}

QPointF ExactLineDialog::end() const
{
    QLineF line(start(), start() + QPointF(1.0, 0.0));
    line.setLength(length());
    line.setAngle(angleDegrees());   // QLineF counts the way paper does
    return line.p2();
}

void ExactLineDialog::updatePreview()
{
    const QPointF to = end();
    m_preview->setText(tr("Ends at X %1, Y %2")
                           .arg(QString::number(to.x(), 'f', 1),
                                QString::number(to.y(), 'f', 1)));
}

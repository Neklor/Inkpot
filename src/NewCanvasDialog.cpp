#include "NewCanvasDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

NewCanvasDialog::NewCanvasDialog(const QSize &initialSize, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Neue Arbeitsflaeche"));

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(16, 20000);
    m_widthSpin->setSuffix(tr(" px"));
    m_widthSpin->setValue(initialSize.width());

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(16, 20000);
    m_heightSpin->setSuffix(tr(" px"));
    m_heightSpin->setValue(initialSize.height());

    m_lockCheck = new QCheckBox(tr("Seitenverhaeltnis beibehalten"), this);
    m_ratioLabel = new QLabel(this);

    if (initialSize.height() > 0)
        m_ratio = double(initialSize.width()) / initialSize.height();

    auto *form = new QFormLayout;
    form->addRow(tr("Breite:"), m_widthSpin);
    form->addRow(tr("Hoehe:"), m_heightSpin);
    form->addRow(QString(), m_lockCheck);
    form->addRow(tr("Verhaeltnis:"), m_ratioLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_widthSpin, &QSpinBox::valueChanged, this, &NewCanvasDialog::widthEdited);
    connect(m_heightSpin, &QSpinBox::valueChanged, this, &NewCanvasDialog::heightEdited);
    connect(m_lockCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && m_heightSpin->value() > 0)
            m_ratio = double(m_widthSpin->value()) / m_heightSpin->value();
    });

    updateRatioLabel();
}

QSize NewCanvasDialog::canvasSize() const
{
    return QSize(m_widthSpin->value(), m_heightSpin->value());
}

bool NewCanvasDialog::aspectRatioLocked() const
{
    return m_lockCheck->isChecked();
}

void NewCanvasDialog::widthEdited(int width)
{
    if (m_updating)
        return;
    m_updating = true;
    if (m_lockCheck->isChecked() && m_ratio > 0.0)
        m_heightSpin->setValue(qMax(16, qRound(width / m_ratio)));
    m_updating = false;
    updateRatioLabel();
}

void NewCanvasDialog::heightEdited(int height)
{
    if (m_updating)
        return;
    m_updating = true;
    if (m_lockCheck->isChecked() && m_ratio > 0.0)
        m_widthSpin->setValue(qMax(16, qRound(height * m_ratio)));
    m_updating = false;
    updateRatioLabel();
}

void NewCanvasDialog::updateRatioLabel()
{
    const int height = m_heightSpin->value();
    if (height <= 0) {
        m_ratioLabel->clear();
        return;
    }
    m_ratioLabel->setText(QStringLiteral("%1").arg(double(m_widthSpin->value()) / height, 0, 'f', 3));
}

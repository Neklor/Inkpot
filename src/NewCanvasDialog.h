#pragma once

#include <QDialog>
#include <QSize>

class QCheckBox;
class QLabel;
class QSpinBox;

// Dialog fuer "Neu > Benutzerdefinierte Groesse".
class NewCanvasDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewCanvasDialog(const QSize &initialSize, QWidget *parent = nullptr);

    QSize canvasSize() const;
    bool aspectRatioLocked() const;

private:
    void widthEdited(int width);
    void heightEdited(int height);
    void updateRatioLabel();

    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QCheckBox *m_lockCheck;
    QLabel *m_ratioLabel;
    double m_ratio = 16.0 / 9.0;
    bool m_updating = false;
};

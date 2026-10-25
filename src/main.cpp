#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Inkpot"));
    QApplication::setApplicationDisplayName(QStringLiteral("Inkpot"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("Inkpot"));

    MainWindow window;
    window.show();

    return app.exec();
}

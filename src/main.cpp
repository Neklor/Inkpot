#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTranslator>

#include "MainWindow.h"

namespace {

// Loads the catalogue for the current locale. Qt's own strings - file dialog,
// standard buttons - live in a separate catalogue shipped with Qt.
void installTranslations(QApplication &app)
{
    const QLocale locale = QLocale::system();

    static QTranslator qtTranslator;
    if (qtTranslator.load(locale, QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }

    // The installed location comes first. The paths next to the executable let
    // a freshly built binary find its translations without being installed.
    const QStringList searchPaths = {
        QStringLiteral(INKPOT_TRANSLATIONS_DIR),
        QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
        QCoreApplication::applicationDirPath(),
    };

    static QTranslator appTranslator;
    for (const QString &path : searchPaths) {
        if (appTranslator.load(locale, QStringLiteral("inkpot"), QStringLiteral("_"), path)) {
            app.installTranslator(&appTranslator);
            return;
        }
    }
}

}   // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Inkpot"));
    QApplication::setApplicationDisplayName(QStringLiteral("Inkpot"));
    QApplication::setApplicationVersion(QStringLiteral(INKPOT_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Inkpot"));

    // Ties the window to its .desktop file, which is how Wayland compositors
    // and application menus find the icon.
    QApplication::setDesktopFileName(QStringLiteral(INKPOT_APP_ID));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral(INKPOT_APP_ID)));

    installTranslations(app);

    // A file given on the command line is how the desktop environment hands
    // over a document that was double-clicked.
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "Draw and edit pictures made of objects."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(
        QCoreApplication::translate("main", "file"),
        QCoreApplication::translate("main", "Project or image to open."));
    parser.process(app);

    MainWindow window;
    window.show();

    const QStringList arguments = parser.positionalArguments();
    if (!arguments.isEmpty())
        window.openPath(arguments.first());

    return app.exec();
}

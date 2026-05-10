// Headless companion binary used by:
//   - themepacksupport-systemupgrade.service (--restore, before a system update)
//   - themepacksupport-autoupdate.service    (--reassert, hourly/daily timer)
//   - sailfishos-uithemer-reassert.service   (--reassert, oneshot at boot)
//
// Loads the JSON manifest at /usr/share/sailfishos-uithemer/icon-backup.json
// and rewrites Icon= in the relevant .desktop files via IconApplier.
//
// No GUI, no QML, no event loop except a short one to flush signals.

#include <QCoreApplication>
#include <QStringList>
#include <QTimer>
#include <QObject>
#include <QDebug>

#include <unistd.h>

#include "iconapplier.h"

static int usage()
{
    qWarning("usage: sailfishos-uithemer-reassert (--reassert | --restore | --refresh-originals)");
    return 2;
}

int main(int argc, char* argv[])
{
    if(setuid(0) != 0) {
        // best-effort; binary is installed setuid-root, so this should succeed
    }

    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();

    if(args.size() < 2)
        return usage();

    const QString mode = args[1];

    IconApplier applier;
    int exitCode = 0;

    QObject::connect(&applier, &IconApplier::applied,            &app, &QCoreApplication::quit);
    QObject::connect(&applier, &IconApplier::restored,           &app, &QCoreApplication::quit);
    QObject::connect(&applier, &IconApplier::reasserted,         &app, &QCoreApplication::quit);
    QObject::connect(&applier, &IconApplier::originalsRefreshed, &app, &QCoreApplication::quit);

    QTimer::singleShot(0, [&]() {
        if(mode == QStringLiteral("--restore"))
            applier.restoreIcons();
        else if(mode == QStringLiteral("--reassert"))
            applier.reassertCurrentTheme();
        else if(mode == QStringLiteral("--refresh-originals"))
            applier.refreshOriginals();
        else {
            exitCode = usage();
            QCoreApplication::quit();
        }
    });

    int rc = app.exec();
    return exitCode != 0 ? exitCode : rc;
}

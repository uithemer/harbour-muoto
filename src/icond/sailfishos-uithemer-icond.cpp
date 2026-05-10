// Headless companion binary used by:
//   - themepacksupport-systemupgrade.service (--restore, before a system update)
//   - themepacksupport-autoupdate.service    (--reassert, hourly/daily timer)
//   - sailfishos-uithemer-icond.service      (--reassert, oneshot at boot)
//
// Loads the JSON manifest at /usr/share/sailfishos-uithemer/icon-backup.json
// and rewrites Icon= in the relevant .desktop files via IconApplier.
//
// Naming note: the trailing -d is historical (this used to be invoked as a
// long-lived agent). Today it is a one-shot CLI; the long-lived privileged
// daemon is /usr/libexec/sailfishos-uithemer-helperd, exposed over the system
// bus. Don't conflate the two.
//
// 2.6.0: the binary itself is no longer setuid root. The 3 systemd
// units that drive it run as root (system units), so the privilege is
// already there when this enters main(); no in-process setuid(0) is
// needed. If a developer manually runs the binary as defaultuser the
// IconApplier writes that touch /usr/share/applications/ will simply
// fail with EACCES, surface as qWarning(), and the manifest is left
// untouched. That's the same outcome as before the setuid was added.

#include <QCoreApplication>
#include <QStringList>
#include <QTimer>
#include <QObject>
#include <QDebug>

#include "iconapplier.h"

static int usage()
{
    qWarning("usage: sailfishos-uithemer-icond (--reassert | --restore | --refresh-originals)");
    return 2;
}

int main(int argc, char* argv[])
{
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

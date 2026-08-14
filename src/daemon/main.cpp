// harbour-muoto-helperd
// ---------------------------
// Privileged D-Bus system service that owns org.muoto.Muoto1 and
// exposes the 2 bus-policy-gated interfaces (Themes, Packs) via
// QDBusAbstractAdaptor subclasses. (2.7.0 retired SystemServices
// together with OptionsPage's autoupdate timer / one-click-restore.)
//
// Lifecycle:
//   1. systemd or dbus-daemon launches us via the .service activation
//      file when defaultuser's GUI first issues a method call.
//   2. We register the well-known name on the system bus.
//   3. Method handlers run DensityEnable / UninstallPack (authorization
//      is the system-bus policy in org.muoto.Muoto1.conf — no polkit).
//   4. After 30 s with no method dispatched we quit; the next call
//      triggers a re-activation.
//
// The binary is intentionally Qt5Core+Qt5Gui+Qt5DBus only (no QML, no
// QtQuick, no SailfishApp): it should never paint a pixel and never
// open the user session bus.

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QStringList>
#include <QObject>
#include <QDebug>

#include "helperservice.h"

namespace
{
    const char* kServiceName = "org.muoto.Muoto1";
    const char* kObjectPath  = "/org/muoto/Muoto1";
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(
        QStringLiteral("harbour-muoto-helperd"));

    // Accept --bus only for forward-compat with the .service file's Exec
    // line; we have no other modes for now. Anything else is ignored.
    Q_UNUSED(app.arguments());

    HelperBackend backend;

    // Adaptors are owned by the backend QObject so registerObject(...,
    // ExportAdaptors) finds them via the parent/child chain.
    new ThemesAdaptor(&backend, &backend);
    new PacksAdaptor(&backend, &backend);

    QDBusConnection bus = QDBusConnection::systemBus();
    if(!bus.isConnected())
    {
        qFatal("muoto-helperd: system bus not available: %s",
               qPrintable(bus.lastError().message()));
        return 1;
    }

    if(!bus.registerObject(QString::fromLatin1(kObjectPath), &backend,
                           QDBusConnection::ExportAdaptors))
    {
        qFatal("muoto-helperd: registerObject failed: %s",
               qPrintable(bus.lastError().message()));
        return 1;
    }

    if(!bus.registerService(QString::fromLatin1(kServiceName)))
    {
        qFatal("muoto-helperd: registerService failed: %s",
               qPrintable(bus.lastError().message()));
        return 1;
    }

    QObject::connect(&backend, &HelperBackend::idleQuit,
                     &app, &QCoreApplication::quit);

    qInfo() << "muoto-helperd: ready on" << kServiceName
            << "@" << kObjectPath;
    return app.exec();
}

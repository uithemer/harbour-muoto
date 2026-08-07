#include "dynamicicon.h"
#include "dynamicicon_p.h"
#include "launchericonops.h"
#include "launchermanifest.h"
#include "launchersettings.h"
#include "launcherpaths.h"
#include "launcherservice.h"

#include <MGConfItem>

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QSocketNotifier>

#include <csignal>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace {

const char* kLauncherService = "org.muoto.Launcher1";
const char* kLauncherPath    = "/org/muoto/Launcher1";

int sigtermFd[2];
QSocketNotifier* termNotifier;

void termSignalHandler(int)
{
    const char a = 1;
    if(write(sigtermFd[0], &a, 1) != 1) { /* ignore */ }
}

void setupSignalHandlers()
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("muoto-launcher-icond: could not create TERM socketpair");

    termNotifier = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read);
    QObject::connect(termNotifier, &QSocketNotifier::activated, []() {
        termNotifier->setEnabled(false);
        char tmp;
        if(read(sigtermFd[1], &tmp, 1) != 1) { /* ignore */ }
        qApp->quit();
        termNotifier->setEnabled(true);
    });

    struct sigaction term;
    term.sa_handler = termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = SA_RESTART;
    if(sigaction(SIGTERM, &term, nullptr))
        qFatal("muoto-launcher-icond: could not setup SIGTERM handler");
}

MGConfItem* activeIconPackConf()
{
    static auto* conf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/activeIconPack"));
    return conf;
}

MGConfItem* iconOverlayConf()
{
    static auto* conf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/iconOverlay"));
    return conf;
}

void checkCapabilities()
{
    // File capabilities on the binary are not always visible via getcap in a
    // minimal service PATH; check the process effective set instead.
    QFile status(QStringLiteral("/proc/self/status"));
    if(!status.open(QIODevice::ReadOnly))
        return;

    const QByteArray data = status.readAll();
    const int idx = data.indexOf("CapEff:");
    if(idx < 0)
        return;

    const QByteArray line = data.mid(idx).split('\n').value(0);
    const QByteArray hex = line.mid(line.indexOf(':') + 1).trimmed();
    bool ok = false;
    const quint64 capEff = hex.toULongLong(&ok, 16);
    // CAP_DAC_OVERRIDE is bit 1
    if(!ok || (capEff & (1ull << 1)) == 0)
        qWarning() << "muoto-launcher-icond: missing cap_dac_override — inplace icon theming may fail";
}

} // namespace

int main(int argc, char* argv[])
{
    if(argc > 1 && std::strcmp(argv[1], "--restore-once") == 0)
    {
        QCoreApplication app(argc, argv);
        const bool ok = LauncherManifest::restoreAll();
        return ok ? 0 : 1;
    }

    QGuiApplication app(argc, argv);

    setupSignalHandlers();
    checkCapabilities();

    QDir().mkpath(LauncherPaths::generatedIconsDir());

    LauncherBackend backend;
    new LauncherThemesAdaptor(&backend, &backend);

    QDBusConnection session = QDBusConnection::sessionBus();
    if(!session.isConnected())
        qFatal("muoto-launcher-icond: session bus not available");

    if(!session.registerObject(QString::fromLatin1(kLauncherPath), &backend,
                               QDBusConnection::ExportAdaptors))
        qFatal("muoto-launcher-icond: registerObject failed");

    if(!session.registerService(QString::fromLatin1(kLauncherService)))
        qWarning() << "muoto-launcher-icond: registerService failed (may already be owned)";

    QObject::connect(&backend, &LauncherBackend::prepareQuit, &app, &QGuiApplication::quit);

    LauncherIconOps* ops = LauncherIconOps::instance();

    QObject::connect(activeIconPackConf(), &MGConfItem::valueChanged, ops, &LauncherIconOps::rebuildIconUpdaters);
    QObject::connect(iconOverlayConf(), &MGConfItem::valueChanged, ops, &LauncherIconOps::rebuildIconUpdaters);

    static auto* dynClockConf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/launcher/dynamicClockEnabled"));
    static auto* dynCalConf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/launcher/dynamicCalendarEnabled"));
    QObject::connect(dynClockConf, &MGConfItem::valueChanged, ops, &LauncherIconOps::rebuildIconUpdaters);
    QObject::connect(dynCalConf, &MGConfItem::valueChanged, ops, &LauncherIconOps::rebuildIconUpdaters);

    for(DynamicIcon* icon : loadDynamicIcons())
        Q_UNUSED(icon);

    if(!LauncherSettings::activeIconPack().isEmpty())
        ops->rebuildIconUpdaters();

    qInfo() << "muoto-launcher-icond: ready on" << kLauncherService;
    return app.exec();
}

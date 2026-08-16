#include "dynamicicon.h"
#include "filelock.h"
#include "iconjob.h"
#include "iconjobqueue.h"
#include "launchericonops.h"
#include "launchermanifest.h"
#include "launcherpaths.h"
#include "launcherservice.h"
#include "launcherwatch.h"
#include "opstatus.h"

#include <MGConfItem>

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QScopedPointer>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QThread>

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
        // Before anything else: a re-arm in flight has launcher entries renamed
        // aside. Quitting on top of that leaves them invisible, and on uninstall
        // no daemon remains to sweep them back.
        LauncherIconOps::instance()->restoreAsideEntries();
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
        // %preun stops the session daemon before calling this, but that stop is
        // best-effort (`systemctl --user` there runs with only XDG_RUNTIME_DIR
        // and its failure is swallowed), so do not rely on it: this writes
        // .desktop files and must not race a daemon that is still alive.
        QScopedPointer<FileLock> lock(new FileLock(FileLock::defaultLockPath(), false));
        for(int waited = 0; !lock->isHeld() && waited < 30; ++waited)
        {
            QThread::sleep(1);
            lock.reset(new FileLock(FileLock::defaultLockPath(), false));
        }
        if(!lock->isHeld())
            qWarning() << "muoto-launcher-icond: --restore-once proceeding without the lock";

        // Anything a killed daemon left renamed aside would otherwise stay that
        // way: this is the last process that can put it back.
        LauncherWatch::sweepStaleRearmFiles(
            {QStringLiteral("/usr/share/applications"),
             QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                 + QStringLiteral("/applications")});

        const bool ok = LauncherManifest::restoreAll();
        return ok ? 0 : 1;
    }

    QGuiApplication app(argc, argv);

    setupSignalHandlers();
    checkCapabilities();
    // Continue the sequence rather than restarting it: the repair oneshot
    // restarts the daemon and then immediately runs update-icons, which
    // compares the sequence to decide whether the result is its own.
    OpStatus::seedSequence();

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

    QObject::connect(&backend, &LauncherBackend::prepareQuit, &app, []() {
        LauncherIconOps::instance()->restoreAsideEntries();
        QGuiApplication::quit();
    });

    LauncherIconOps* ops = LauncherIconOps::instance();
    Q_UNUSED(ops);

    // A dconf change means "re-attach updaters", which is a queued job like any
    // other. It used to be dropped outright when an operation was in flight.
    const auto enqueueRebuild = [](const QString& key, const QString& value) {
        if(IconJobQueue::instance()->isSelfWrite(key, value))
            return;
        IconJob job;
        job.kind = IconJob::Rebuild;
        IconJobQueue::instance()->enqueue(job);
    };

    QObject::connect(activeIconPackConf(), &MGConfItem::valueChanged, qApp, [enqueueRebuild]() {
        enqueueRebuild(QStringLiteral("activeIconPack"),
                       activeIconPackConf()->value().toString());
    });
    QObject::connect(iconOverlayConf(), &MGConfItem::valueChanged, qApp, [enqueueRebuild]() {
        enqueueRebuild(QStringLiteral("iconOverlay"),
                       iconOverlayConf()->value().toBool() ? QStringLiteral("true")
                                                           : QStringLiteral("false"));
    });

    static auto* dynClockConf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/launcher/dynamicClockEnabled"));
    static auto* dynCalConf = new MGConfItem(QStringLiteral("/apps/harbour-muoto/launcher/dynamicCalendarEnabled"));
    QObject::connect(dynClockConf, &MGConfItem::valueChanged, qApp, [enqueueRebuild]() {
        enqueueRebuild(QStringLiteral("dynamicClockEnabled"), QString());
    });
    QObject::connect(dynCalConf, &MGConfItem::valueChanged, qApp, [enqueueRebuild]() {
        enqueueRebuild(QStringLiteral("dynamicCalendarEnabled"), QString());
    });

    // Launcher icon geometry: nothing watched these, so after a density change
    // the pack index and every resolved path stayed keyed to the old size until
    // the daemon happened to restart.
    static auto* pixelRatioConf = new MGConfItem(QStringLiteral("/desktop/sailfish/silica/theme_pixel_ratio"));
    static auto* iconSizeConf = new MGConfItem(QStringLiteral("/desktop/sailfish/silica/icon_size_launcher"));
    QObject::connect(pixelRatioConf, &MGConfItem::valueChanged, qApp, [enqueueRebuild]() {
        enqueueRebuild(QStringLiteral("theme_pixel_ratio"), QString());
    });
    QObject::connect(iconSizeConf, &MGConfItem::valueChanged, qApp, [enqueueRebuild]() {
        // The density dialog writes both keys in one accept; the queue coalesces
        // them into a single rebuild.
        enqueueRebuild(QStringLiteral("icon_size_launcher"), QString());
    });

    for(DynamicIcon* icon : loadDynamicIcons())
        Q_UNUSED(icon);

    // Pack and/or stock dynamic clock/calendar (works with activeIconPack=default).
    {
        IconJob job;
        job.kind = IconJob::Rebuild;
        IconJobQueue::instance()->enqueue(job);
    }

    qInfo() << "muoto-launcher-icond: ready on" << kLauncherService;
    return app.exec();
}

#include "helperclient.h"
#include "filelock.h"
#include "launcherdaemonctl.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusError>
#include <QElapsedTimer>
#include <QTimer>
#include <QDebug>
#include <QQmlEngine>
#include <QJSEngine>

namespace
{
    const char* kRootServiceName = "org.muoto.Muoto1";
    const char* kRootObjectPath  = "/org/muoto/Muoto1";
    const char* kRootIfaceThemes  = "org.muoto.Muoto1.Themes";
    const char* kRootIfacePacks   = "org.muoto.Muoto1.Packs";

    const char* kLauncherServiceName = "org.muoto.Launcher1";
    const char* kLauncherObjectPath  = "/org/muoto/Launcher1";
    const char* kLauncherIfaceThemes = "org.muoto.Launcher1.Themes";

    // Poll cadence and budget for a launcher daemon we just kicked.
    const int kLauncherPollMs  = 250;
    const int kLauncherGiveUpMs = 15000;

    // Silence budget for an inflight op. Restarted on every Progress, and the
    // daemon emits one on enqueue and through re-arm and the holdback, so this
    // only elapses when nothing is coming back at all.
    const int kOpSilenceMs = 90000;

    struct Endpoint
    {
        const char* service;
        const char* path;
        const char* iface;
        bool systemBus;
    };

    bool endpointForOp(const QString& op, Endpoint* out)
    {
        if(op == QLatin1String("ApplyIcons") || op == QLatin1String("RestoreIcons"))
        {
            *out = { kLauncherServiceName, kLauncherObjectPath,
                     kLauncherIfaceThemes, false };
            return true;
        }
        if(op == QLatin1String("DensityEnable"))
        {
            *out = { kRootServiceName, kRootObjectPath, kRootIfaceThemes, true };
            return true;
        }
        if(op == QLatin1String("UninstallPack"))
        {
            *out = { kRootServiceName, kRootObjectPath, kRootIfacePacks, true };
            return true;
        }
        return false;
    }
}

HelperClient::HelperClient()
    : QObject(nullptr)
    , _launcherWait(new QTimer(this))
    , _opWatchdog(new QTimer(this))
    , _launcherWaitedMs(0)
    , _hooked(false)
{
    _launcherWait->setInterval(kLauncherPollMs);
    connect(_launcherWait, &QTimer::timeout, this, &HelperClient::onLauncherWaitTick);

    _opWatchdog->setInterval(kOpSilenceMs);
    _opWatchdog->setSingleShot(true);
    connect(_opWatchdog, &QTimer::timeout, this, &HelperClient::onOpWatchdog);

    hookBroadcastSignals();

    // Warm the session daemon at startup so the first apply finds it on the
    // bus instead of paying for the unit start.
    if(!launcherDaemonRegistered())
        startLauncherDaemonDetached();
}

HelperClient::~HelperClient() = default;

HelperClient* HelperClient::instance()
{
    static HelperClient* s_instance = new HelperClient();
    return s_instance;
}

QObject* HelperClient::qmlSingleton(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    HelperClient* h = instance();
    QQmlEngine::setObjectOwnership(h, QQmlEngine::CppOwnership);
    return h;
}

void HelperClient::hookBroadcastSignals()
{
    if(_hooked)
        return;
    _hooked = true;

    QDBusConnection systemBus = QDBusConnection::systemBus();
    if(systemBus.isConnected())
    {
        systemBus.connect(QString::fromLatin1(kRootServiceName),
                          QString::fromLatin1(kRootObjectPath),
                          QString::fromLatin1(kRootIfaceThemes),
                          QStringLiteral("OperationCompleted"),
                          this,
                          SLOT(onThemesOperationCompleted(QString, bool, QString)));

        systemBus.connect(QString::fromLatin1(kRootServiceName),
                          QString::fromLatin1(kRootObjectPath),
                          QString::fromLatin1(kRootIfacePacks),
                          QStringLiteral("OperationCompleted"),
                          this,
                          SLOT(onPacksOperationCompleted(QString, bool, QString)));
    }

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if(sessionBus.isConnected())
    {
        sessionBus.connect(QString::fromLatin1(kLauncherServiceName),
                           QString::fromLatin1(kLauncherObjectPath),
                           QString::fromLatin1(kLauncherIfaceThemes),
                           QStringLiteral("OperationCompleted"),
                           this,
                           SLOT(onThemesOperationCompleted(QString, bool, QString)));

        sessionBus.connect(QString::fromLatin1(kLauncherServiceName),
                           QString::fromLatin1(kLauncherObjectPath),
                           QString::fromLatin1(kLauncherIfaceThemes),
                           QStringLiteral("Progress"),
                           this,
                           SLOT(onLauncherProgress(QString, int, int)));
    }
    else
    {
        qWarning() << "HelperClient: session bus not connected";
    }
}

void HelperClient::asyncCall(const QString& op, const QVariantList& args)
{
    QElapsedTimer t;
    t.start();

    Endpoint ep;
    if(!endpointForOp(op, &ep))
    {
        qWarning() << "HelperClient::asyncCall: unknown op" << op;
        emit error(op, QStringLiteral("D-Bus interface unavailable"));
        return;
    }

    QDBusConnection bus = ep.systemBus ? QDBusConnection::systemBus()
                                       : QDBusConnection::sessionBus();
    if(!bus.isConnected())
    {
        qWarning() << "HelperClient::asyncCall:" << op << "bus not connected";
        emit error(op, QStringLiteral("D-Bus interface unavailable"));
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
                QString::fromLatin1(ep.service),
                QString::fromLatin1(ep.path),
                QString::fromLatin1(ep.iface),
                op);
    call.setArguments(args);

    QDBusPendingCall pending = bus.asyncCall(call);
    auto* w = new QDBusPendingCallWatcher(pending, this);
    connect(w, &QDBusPendingCallWatcher::finished, this,
            [this, op](QDBusPendingCallWatcher* watcher) {
        QDBusPendingReply<> r = *watcher;
        if(r.isError())
        {
            // No OperationCompleted will follow a transport failure, so the
            // GUI needs this to drain its busy state.
            qWarning() << "HelperClient::asyncCall:" << op
                       << "method reply:" << r.error().message();
            emit error(op, QStringLiteral("D-Bus interface unavailable"));
        }
        watcher->deleteLater();
    });

    qInfo() << "HelperClient: dispatched" << op << "in" << t.elapsed() << "ms";
}

void HelperClient::queueIconOp(const QString& op, const QVariantList& args)
{
    // Latest request wins if we are still waiting for the daemon.
    _pendingIconOp = op;
    _pendingIconArgs = args;

    // No lock probe: the daemon serialises, and it reports a queued job as
    // indeterminate progress, which is both accurate and something the client
    // could never work out by sampling a lock.
    dispatchPendingIconOp();
}

void HelperClient::dispatchPendingIconOp()
{
    const QString op = _pendingIconOp;
    const QVariantList args = _pendingIconArgs;
    if(op.isEmpty())
        return;

    if(launcherDaemonRegistered())
    {
        _inflightIconOp = op;
        _inflightIconArgs = args;
        _pendingIconOp.clear();
        _pendingIconArgs.clear();
        _launcherWait->stop();
        _opWatchdog->start();
        qInfo() << "HelperClient: dispatching" << op;
        asyncCall(op, args);
        return;
    }

    if(_launcherWait->isActive())
        return;

    qInfo() << "HelperClient:" << op << "launcher daemon absent, starting detached";
    _launcherWaitedMs = 0;
    startLauncherDaemonDetached();
    _launcherWait->start();
}

void HelperClient::failPendingIconOp(const QString& message)
{
    _opWatchdog->stop();
    _launcherWait->stop();
    QString op = _pendingIconOp;
    if(op.isEmpty())
        op = _inflightIconOp;
    _pendingIconOp.clear();
    _pendingIconArgs.clear();
    _inflightIconOp.clear();
    _inflightIconArgs.clear();
    if(!op.isEmpty())
        emit error(op, message);
}

void HelperClient::onOpWatchdog()
{
    if(_inflightIconOp.isEmpty())
        return;

    qWarning() << "HelperClient:" << _inflightIconOp << "silent for" << kOpSilenceMs
               << "ms; giving up";
    failPendingIconOp(QStringLiteral("timed out waiting for icon operation"));
}

void HelperClient::onLauncherWaitTick()
{
    if(launcherDaemonRegistered())
    {
        _launcherWait->stop();
        qInfo() << "HelperClient: launcher daemon up after" << _launcherWaitedMs
                << "ms";
        dispatchPendingIconOp();
        return;
    }

    _launcherWaitedMs += kLauncherPollMs;
    if(_launcherWaitedMs < kLauncherGiveUpMs)
        return;

    qWarning() << "HelperClient:" << _pendingIconOp << "launcher daemon not running";
    failPendingIconOp(QStringLiteral("launcher daemon not running"));
}

void HelperClient::applyIcons(const QString& pack, bool runPack, bool overlay)
{
    queueIconOp(QStringLiteral("ApplyIcons"),
                QVariantList() << pack << runPack << overlay);
}

void HelperClient::restoreIcons()
{
    queueIconOp(QStringLiteral("RestoreIcons"), QVariantList());
}

void HelperClient::densityEnable()
{
    // org.muoto.Muoto1 is dbus-activatable (system-services + SystemdService),
    // so the bus starts helperd for us while the call is in flight.
    asyncCall(QStringLiteral("DensityEnable"), QVariantList());
}

void HelperClient::uninstallPack(const QString& rpmName)
{
    asyncCall(QStringLiteral("UninstallPack"), QVariantList() << rpmName);
}

void HelperClient::onLauncherProgress(const QString& op, int done, int total)
{
    // Any progress is proof of life: restart the silence budget so a job that is
    // slow, or queued behind a drain, is not mistaken for a dead daemon.
    if(!_inflightIconOp.isEmpty() && op == _inflightIconOp)
        _opWatchdog->start();

    emit iconProgress(op, done, total);
}

void HelperClient::onThemesOperationCompleted(const QString& op, bool ok,
                                              const QString& message)
{
    // ApplyIcons and RestoreIcons also arrive unsolicited: the install listener
    // re-themes in the background, and OperationCompleted is a broadcast. Acting
    // on those is what put a toast on screen with the app closed. Only ops this
    // process dispatched are ours.
    //
    // Scoped to those two on purpose. DensityEnable shares this slot (helperd's
    // Themes interface on the system bus) but goes out through asyncCall without
    // ever setting _inflightIconOp, so a blanket check would strand the density
    // dialog waiting for a completion it already had.
    const bool iconOp = op == QLatin1String("ApplyIcons") || op == QLatin1String("RestoreIcons");
    if(iconOp && op != _inflightIconOp)
    {
        qInfo() << "HelperClient: ignoring unsolicited" << op << "ok=" << ok;
        return;
    }

    // The busy re-queue branch is gone with the probe: the daemon queues rather
    // than rejecting, and waits out another process holding the lock.
    _opWatchdog->stop();

    if(!ok)
    {
        _inflightIconOp.clear();
        _inflightIconArgs.clear();
        emit error(op, message);
        return;
    }
    _inflightIconOp.clear();
    _inflightIconArgs.clear();
    if(op == QLatin1String("ApplyIcons"))
    {
        Q_UNUSED(message);
        emit iconsApplied();
    }
    else if(op == QLatin1String("RestoreIcons"))
    {
        Q_UNUSED(message);
        emit iconsRestored();
    }
    else if(op == QLatin1String("DensityEnable"))
        emit densityEnabled();
    else
        qWarning() << "HelperClient: unknown Themes op" << op;
}

void HelperClient::onPacksOperationCompleted(const QString& op, bool ok,
                                             const QString& message)
{
    if(!ok)
    {
        emit error(op, message);
        return;
    }
    if(op == QLatin1String("UninstallPack"))
        emit packUninstalled(message);
    else
        qWarning() << "HelperClient: unknown Packs op" << op;
}

#include "helperclient.h"
#include "filelock.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include <QDBusError>
#include <QDBusConnectionInterface>
#include <QVariantList>
#include <QDebug>
#include <QQmlEngine>
#include <QJSEngine>

namespace
{
    const char* kServiceName = "org.uithemer.UiThemer1";
    const char* kObjectPath  = "/org/uithemer/UiThemer1";
    const char* kIfaceThemes  = "org.uithemer.UiThemer1.Themes";
    const char* kIfacePacks   = "org.uithemer.UiThemer1.Packs";
}

HelperClient::HelperClient()
    : QObject(nullptr)
    , _themes(nullptr)
    , _packs(nullptr)
    , _hooked(false)
{
    // Subscribe to the daemon's broadcast signals as early as possible
    // so we never miss an OperationCompleted that races our asyncCall.
    hookBroadcastSignals();

    QDBusConnection bus = QDBusConnection::systemBus();
    if(bus.isConnected())
    {
        bus.connect(QStringLiteral("org.freedesktop.DBus"),
                    QStringLiteral("/org/freedesktop/DBus"),
                    QStringLiteral("org.freedesktop.DBus"),
                    QStringLiteral("NameOwnerChanged"),
                    this,
                    SLOT(onNameOwnerChanged(QString, QString, QString)));
    }
}

HelperClient::~HelperClient()
{
    delete _themes;
    delete _packs;
}

HelperClient* HelperClient::instance()
{
    // GUI is strictly single-threaded (Qt main thread), so plain static
    // initialisation is sufficient — no Q_GLOBAL_STATIC needed.
    static HelperClient* s_instance = new HelperClient();
    return s_instance;
}

QObject* HelperClient::qmlSingleton(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    HelperClient* h = instance();
    // The QML engine must not own / GC the singleton — we share it
    // with C++ peers (ThemePackModel, ThemePack) and its lifetime is
    // the whole process.
    QQmlEngine::setObjectOwnership(h, QQmlEngine::CppOwnership);
    return h;
}

bool HelperClient::ensureHelperService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if(!bus.isConnected())
        return false;
    QDBusConnectionInterface* bi = bus.interface();
    if(!bi)
        return false;
    const QString svc = QString::fromLatin1(kServiceName);
    const QDBusReply<bool> registered = bi->isServiceRegistered(svc);
    if(registered.isValid() && registered.value())
        return true;
    const QDBusReply<void> started = bi->startService(svc);
    return started.isValid();
}

void HelperClient::dropDBusProxies()
{
    delete _themes;
    _themes = nullptr;
    delete _packs;
    _packs = nullptr;
}

void HelperClient::onNameOwnerChanged(const QString& name,
                                      const QString& oldOwner,
                                      const QString& newOwner)
{
    if(name != QLatin1String(kServiceName))
        return;
    if(!newOwner.isEmpty() || oldOwner.isEmpty())
        return;

    // A helper instance released the name. dbus-activation may already have
    // started a replacement; ignore stale "owner lost" signals in that case.
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusConnectionInterface* bi = bus.interface();
    const QDBusReply<bool> registered =
        bi ? bi->isServiceRegistered(QString::fromLatin1(kServiceName))
           : QDBusReply<bool>();
    if(registered.isValid() && registered.value())
        return;

    dropDBusProxies();
}

QDBusInterface* HelperClient::themesIface()
{
    ensureHelperService();
    if(_themes && !_themes->isValid())
    {
        delete _themes;
        _themes = nullptr;
    }
    if(!_themes)
    {
        _themes = new QDBusInterface(QString::fromLatin1(kServiceName),
                                     QString::fromLatin1(kObjectPath),
                                     QString::fromLatin1(kIfaceThemes),
                                     QDBusConnection::systemBus(), this);
    }
    return _themes;
}

QDBusInterface* HelperClient::packsIface()
{
    ensureHelperService();
    if(_packs && !_packs->isValid())
    {
        delete _packs;
        _packs = nullptr;
    }
    if(!_packs)
        _packs = new QDBusInterface(QString::fromLatin1(kServiceName),
                                    QString::fromLatin1(kObjectPath),
                                    QString::fromLatin1(kIfacePacks),
                                    QDBusConnection::systemBus(), this);
    return _packs;
}

void HelperClient::hookBroadcastSignals()
{
    if(_hooked)
        return;
    _hooked = true;

    QDBusConnection bus = QDBusConnection::systemBus();
    if(!bus.isConnected())
    {
        qWarning() << "HelperClient: system bus not connected";
        return;
    }

    bus.connect(QString::fromLatin1(kServiceName),
                QString::fromLatin1(kObjectPath),
                QString::fromLatin1(kIfaceThemes),
                QStringLiteral("OperationCompleted"),
                this,
                SLOT(onThemesOperationCompleted(QString, bool, QString)));

    bus.connect(QString::fromLatin1(kServiceName),
                QString::fromLatin1(kObjectPath),
                QString::fromLatin1(kIfaceThemes),
                QStringLiteral("Progress"),
                this,
                SLOT(onThemesProgress(QString, int, int)));

    bus.connect(QString::fromLatin1(kServiceName),
                QString::fromLatin1(kObjectPath),
                QString::fromLatin1(kIfacePacks),
                QStringLiteral("OperationCompleted"),
                this,
                SLOT(onPacksOperationCompleted(QString, bool, QString)));
}

void HelperClient::asyncCall(const QString& op, const QVariantList& args)
{
    const bool themesOp =
        (op == QLatin1String("ApplyIcons")
         || op == QLatin1String("RestoreIcons")
         || op == QLatin1String("RefreshOriginals")
         || op == QLatin1String("DensityEnable"));
    QDBusInterface* iface = themesOp ? themesIface()
                        : (op == QLatin1String("UninstallPack") ? packsIface()
                                                                : nullptr);

    if(!iface || !iface->isValid())
    {
        qWarning() << "HelperClient::asyncCall:" << op
                   << "iface invalid:"
                   << (iface ? iface->lastError().message()
                             : QStringLiteral("null"));
        emit error(op, QStringLiteral("D-Bus interface unavailable"));
        return;
    }

    QDBusPendingCall pending = iface->asyncCallWithArgumentList(op, args);
    QDBusPendingCallWatcher* w = new QDBusPendingCallWatcher(pending, this);
    connect(w, &QDBusPendingCallWatcher::finished, this,
            [op](QDBusPendingCallWatcher* watcher) {
        // Method-reply errors only happen if the daemon explicitly
        // sendErrorReply()s or if the bus transport itself fails;
        // success is reported via the broadcast signal, not the reply.
        QDBusPendingReply<> r = *watcher;
        if(r.isError())
        {
            // Real success/failure is reported via OperationCompleted;
            // missing method replies must not surface as op failures.
            qWarning() << "HelperClient::asyncCall:" << op
                       << "method reply:" << r.error().message();
        }
        watcher->deleteLater();
    });
}

// ---- Themes ---------------------------------------------------------------

bool HelperClient::beginIconOpOrError(const QString& op)
{
    if(!FileLock::tryProbe())
    {
        emit error(op, QStringLiteral("busy"));
        return false;
    }
    return true;
}

void HelperClient::applyIcons(const QString& pack, bool runPack, bool overlay)
{
    const QString op = QStringLiteral("ApplyIcons");
    if(!beginIconOpOrError(op))
        return;
    asyncCall(op, QVariantList() << pack << runPack << overlay);
}

void HelperClient::restoreIcons()
{
    const QString op = QStringLiteral("RestoreIcons");
    if(!beginIconOpOrError(op))
        return;
    asyncCall(op, QVariantList());
}

void HelperClient::refreshOriginals()
{
    const QString op = QStringLiteral("RefreshOriginals");
    if(!beginIconOpOrError(op))
        return;
    asyncCall(op, QVariantList());
}

void HelperClient::densityEnable()
{
    asyncCall(QStringLiteral("DensityEnable"), QVariantList());
}

// ---- Packs ----------------------------------------------------------------

void HelperClient::uninstallPack(const QString& rpmName)
{
    asyncCall(QStringLiteral("UninstallPack"), QVariantList() << rpmName);
}

// ---- Demux of broadcast signals -------------------------------------------

void HelperClient::onThemesOperationCompleted(const QString& op, bool ok,
                                              const QString& message)
{
    if(!ok)
    {
        emit error(op, message);
        return;
    }
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
    else if(op == QLatin1String("RefreshOriginals"))
    {
        Q_UNUSED(message);
        emit originalsRefreshed();
    }
    else if(op == QLatin1String("DensityEnable"))
        emit densityEnabled();
    else
        qWarning() << "HelperClient: unknown Themes op" << op;
}

void HelperClient::onThemesProgress(const QString& op, int done, int total)
{
    Q_UNUSED(op);
    emit progress(done, total);
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

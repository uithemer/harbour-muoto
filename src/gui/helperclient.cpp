#include "helperclient.h"
#include "filelock.h"
#include "launcherdaemonctl.h"

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
    const char* kRootServiceName = "org.muoto.Muoto1";
    const char* kRootObjectPath  = "/org/muoto/Muoto1";
    const char* kRootIfaceThemes  = "org.muoto.Muoto1.Themes";
    const char* kRootIfacePacks   = "org.muoto.Muoto1.Packs";

    const char* kLauncherServiceName = "org.muoto.Launcher1";
    const char* kLauncherObjectPath  = "/org/muoto/Launcher1";
    const char* kLauncherIfaceThemes = "org.muoto.Launcher1.Themes";
}

HelperClient::HelperClient()
    : QObject(nullptr)
    , _themes(nullptr)
    , _launcherThemes(nullptr)
    , _packs(nullptr)
    , _hooked(false)
{
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
    delete _launcherThemes;
    delete _packs;
}

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

bool HelperClient::ensureHelperService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if(!bus.isConnected())
        return false;
    QDBusConnectionInterface* bi = bus.interface();
    if(!bi)
        return false;
    const QString svc = QString::fromLatin1(kRootServiceName);
    const QDBusReply<bool> registered = bi->isServiceRegistered(svc);
    if(registered.isValid() && registered.value())
        return true;
    const QDBusReply<void> started = bi->startService(svc);
    return started.isValid();
}

bool HelperClient::ensureLauncherService()
{
    return ensureLauncherDaemonRunning();
}

void HelperClient::dropDBusProxies()
{
    delete _themes;
    _themes = nullptr;
    delete _launcherThemes;
    _launcherThemes = nullptr;
    delete _packs;
    _packs = nullptr;
}

void HelperClient::onNameOwnerChanged(const QString& name,
                                      const QString& oldOwner,
                                      const QString& newOwner)
{
    if(name != QLatin1String(kRootServiceName)
       && name != QLatin1String(kLauncherServiceName))
        return;
    if(!newOwner.isEmpty() || oldOwner.isEmpty())
        return;

    QDBusConnection bus = name == QLatin1String(kRootServiceName)
                              ? QDBusConnection::systemBus()
                              : QDBusConnection::sessionBus();
    QDBusConnectionInterface* bi = bus.interface();
    const QDBusReply<bool> registered =
        bi ? bi->isServiceRegistered(name) : QDBusReply<bool>();
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
        _themes = new QDBusInterface(QString::fromLatin1(kRootServiceName),
                                     QString::fromLatin1(kRootObjectPath),
                                     QString::fromLatin1(kRootIfaceThemes),
                                     QDBusConnection::systemBus(), this);
    }
    return _themes;
}

QDBusInterface* HelperClient::launcherThemesIface()
{
    ensureLauncherService();
    if(_launcherThemes && !_launcherThemes->isValid())
    {
        delete _launcherThemes;
        _launcherThemes = nullptr;
    }
    if(!_launcherThemes)
    {
        _launcherThemes = new QDBusInterface(QString::fromLatin1(kLauncherServiceName),
                                             QString::fromLatin1(kLauncherObjectPath),
                                             QString::fromLatin1(kLauncherIfaceThemes),
                                             QDBusConnection::sessionBus(), this);
    }
    return _launcherThemes;
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
    {
        _packs = new QDBusInterface(QString::fromLatin1(kRootServiceName),
                                    QString::fromLatin1(kRootObjectPath),
                                    QString::fromLatin1(kRootIfacePacks),
                                    QDBusConnection::systemBus(), this);
    }
    return _packs;
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
                          QString::fromLatin1(kRootIfaceThemes),
                          QStringLiteral("Progress"),
                          this,
                          SLOT(onThemesProgress(QString, int, int)));

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
                           SLOT(onThemesProgress(QString, int, int)));
    }
    else
    {
        qWarning() << "HelperClient: session bus not connected";
    }
}

void HelperClient::asyncCall(const QString& op, const QVariantList& args)
{
    QDBusInterface* iface = nullptr;
    if(op == QLatin1String("ApplyIcons") || op == QLatin1String("RestoreIcons"))
        iface = launcherThemesIface();
    else if(op == QLatin1String("DensityEnable"))
        iface = themesIface();
    else if(op == QLatin1String("UninstallPack"))
        iface = packsIface();

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
        QDBusPendingReply<> r = *watcher;
        if(r.isError())
        {
            qWarning() << "HelperClient::asyncCall:" << op
                       << "method reply:" << r.error().message();
        }
        watcher->deleteLater();
    });
}

bool HelperClient::beginIconOpOrError(const QString& op)
{
    if(!FileLock::tryProbe())
    {
        emit error(op, QStringLiteral("busy"));
        return false;
    }
    if(!ensureLauncherService())
    {
        qWarning() << "HelperClient:" << op << "launcher daemon not running";
        emit error(op, QStringLiteral("launcher daemon not running"));
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

void HelperClient::densityEnable()
{
    asyncCall(QStringLiteral("DensityEnable"), QVariantList());
}

void HelperClient::uninstallPack(const QString& rpmName)
{
    asyncCall(QStringLiteral("UninstallPack"), QVariantList() << rpmName);
}

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

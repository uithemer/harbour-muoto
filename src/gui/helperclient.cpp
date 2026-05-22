#include "helperclient.h"
#include "filelock.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include <QDBusError>
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

QDBusInterface* HelperClient::themesIface()
{
    if(!_themes)
        _themes = new QDBusInterface(QString::fromLatin1(kServiceName),
                                     QString::fromLatin1(kObjectPath),
                                     QString::fromLatin1(kIfaceThemes),
                                     QDBusConnection::systemBus(), this);
    return _themes;
}

QDBusInterface* HelperClient::packsIface()
{
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

void HelperClient::asyncCall(QDBusInterface* iface, const QString& op,
                             const QVariantList& args)
{
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
            [this, op](QDBusPendingCallWatcher* watcher) {
        // Method-reply errors only happen if the daemon explicitly
        // sendErrorReply()s or if the bus transport itself fails;
        // success is reported via the broadcast signal, not the reply.
        QDBusPendingReply<> r = *watcher;
        if(r.isError())
        {
            qWarning() << "HelperClient::asyncCall:" << op
                       << "error:" << r.error().message();
            emit error(op, r.error().message());
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

void HelperClient::applyIcons(const QString& pack, bool overlay)
{
    const QString op = QStringLiteral("ApplyIcons");
    if(!beginIconOpOrError(op))
        return;
    asyncCall(themesIface(), op, QVariantList() << pack << overlay);
}

void HelperClient::restoreIcons()
{
    const QString op = QStringLiteral("RestoreIcons");
    if(!beginIconOpOrError(op))
        return;
    asyncCall(themesIface(), op, QVariantList());
}

void HelperClient::refreshOriginals()
{
    const QString op = QStringLiteral("RefreshOriginals");
    if(!beginIconOpOrError(op))
        return;
    asyncCall(themesIface(), op, QVariantList());
}

void HelperClient::densityEnable()
{
    asyncCall(themesIface(), QStringLiteral("DensityEnable"), QVariantList());
}

// ---- Packs ----------------------------------------------------------------

void HelperClient::uninstallPack(const QString& rpmName)
{
    asyncCall(packsIface(), QStringLiteral("UninstallPack"),
              QVariantList() << rpmName);
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

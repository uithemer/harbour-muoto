#include "helperclient.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include <QDBusError>
#include <QVariantList>
#include <QDebug>

namespace
{
    const char* kServiceName = "org.uithemer.UiThemer1";
    const char* kObjectPath  = "/org/uithemer/UiThemer1";
    const char* kIfaceThemes  = "org.uithemer.UiThemer1.Themes";
    const char* kIfacePacks   = "org.uithemer.UiThemer1.Packs";
    const char* kIfaceSystem  = "org.uithemer.UiThemer1.SystemServices";
}

HelperClient::HelperClient(QObject* parent)
    : QObject(parent)
    , _themes(nullptr)
    , _packs(nullptr)
    , _systemServices(nullptr)
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
    delete _systemServices;
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

QDBusInterface* HelperClient::systemServicesIface()
{
    if(!_systemServices)
        _systemServices = new QDBusInterface(QString::fromLatin1(kServiceName),
                                             QString::fromLatin1(kObjectPath),
                                             QString::fromLatin1(kIfaceSystem),
                                             QDBusConnection::systemBus(), this);
    return _systemServices;
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

    bus.connect(QString::fromLatin1(kServiceName),
                QString::fromLatin1(kObjectPath),
                QString::fromLatin1(kIfaceSystem),
                QStringLiteral("OperationCompleted"),
                this,
                SLOT(onSystemServicesOperationCompleted(QString, bool, QString)));
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

void HelperClient::applyIcons(const QString& pack, bool overlay)
{
    asyncCall(themesIface(), QStringLiteral("ApplyIcons"),
              QVariantList() << pack << overlay);
}

void HelperClient::restoreIcons()
{
    asyncCall(themesIface(), QStringLiteral("RestoreIcons"), QVariantList());
}

void HelperClient::reassertIcons()
{
    asyncCall(themesIface(), QStringLiteral("ReassertIcons"), QVariantList());
}

void HelperClient::refreshOriginals()
{
    asyncCall(themesIface(), QStringLiteral("RefreshOriginals"), QVariantList());
}

void HelperClient::themeNewDesktops()
{
    asyncCall(themesIface(), QStringLiteral("ThemeNewDesktops"), QVariantList());
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

// ---- SystemServices -------------------------------------------------------

void HelperClient::hideIcon()
{
    asyncCall(systemServicesIface(), QStringLiteral("HideIcon"), QVariantList());
}

void HelperClient::setAutoupdate(bool enabled)
{
    asyncCall(systemServicesIface(), QStringLiteral("SetAutoupdate"),
              QVariantList() << enabled);
}

void HelperClient::setServiceSu(bool enabled)
{
    asyncCall(systemServicesIface(), QStringLiteral("SetServiceSu"),
              QVariantList() << enabled);
}

void HelperClient::applyHours(const QString& hours)
{
    asyncCall(systemServicesIface(), QStringLiteral("ApplyHours"),
              QVariantList() << hours);
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
        emit iconsApplied();
    else if(op == QLatin1String("RestoreIcons"))
        emit iconsRestored();
    else if(op == QLatin1String("ReassertIcons"))
        emit iconsReasserted();
    else if(op == QLatin1String("RefreshOriginals"))
        emit originalsRefreshed();
    else if(op == QLatin1String("ThemeNewDesktops"))
        emit newDesktopsThemed(message.toInt());
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

void HelperClient::onSystemServicesOperationCompleted(const QString& op,
                                                      bool ok,
                                                      const QString& message)
{
    if(!ok)
    {
        emit error(op, message);
        return;
    }
    if(op == QLatin1String("HideIcon"))
        emit hideIconDone();
    else if(op == QLatin1String("SetAutoupdate")
         || op == QLatin1String("SetServiceSu"))
        emit serviceChanged();
    else if(op == QLatin1String("ApplyHours"))
        emit hoursApplied(message);
    else
        qWarning() << "HelperClient: unknown SystemServices op" << op;
}

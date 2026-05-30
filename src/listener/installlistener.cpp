#include "installlistener.h"
#include "pktxwatch.h"

#include <QDBusServiceWatcher>
#include <QFile>
#include <QDebug>

namespace
{
    const char* kApkdService = "com.jolla.apkd";
    const char* kApkdPath = "/com/jolla/apkd";
    const char* kApkdIface = "com.jolla.apkd";

    const char* kInstService = "org.sailfishos.installationhandler";
    const char* kInstPath = "/org/sailfishos/installationhandler";
    const char* kInstIface = "org.sailfishos.installationhandler";

    const char* kPkService = "org.freedesktop.PackageKit";
    const char* kPkPath = "/org/freedesktop/PackageKit";
    const char* kPkIface = "org.freedesktop.PackageKit";
    const char* kPkTxIface = "org.freedesktop.PackageKit.Transaction";

    const char* kLogin1Service = "org.freedesktop.login1";
    const char* kLogin1Path = "/org/freedesktop/login1";
    const char* kLogin1Mgr = "org.freedesktop.login1.Manager";

    const char* kUpdateScript = "/usr/bin/harbour-muoto-update-icons";
    const char* kOsUpdateSentinel = "/tmp/os-update-running";

    constexpr uint PK_EXIT_SUCCESS = 1;
}

InstallListener::InstallListener(QObject* parent)
    : QObject(parent)
    , _session(QDBusConnection::sessionBus())
    , _system(QDBusConnection::systemBus())
{
    _debounce.setSingleShot(true);
    _debounce.setInterval(1500);
    connect(&_debounce, &QTimer::timeout, this, &InstallListener::onDebounceTimeout);

    subscribeSession();
    subscribeSystem();

    qInfo() << "muoto-listener: started";

    auto* apkdWatch =
        new QDBusServiceWatcher(QString::fromLatin1(kApkdService),
                                _session,
                                QDBusServiceWatcher::WatchForOwnerChange,
                                this);
    connect(apkdWatch, &QDBusServiceWatcher::serviceOwnerChanged,
            this, [this](const QString&, const QString&, const QString&) {
                qInfo() << "muoto-listener: apkd owner changed, re-subscribing";
                subscribeSession();
            });

    auto* instWatch =
        new QDBusServiceWatcher(QString::fromLatin1(kInstService),
                                _session,
                                QDBusServiceWatcher::WatchForOwnerChange,
                                this);
    connect(instWatch, &QDBusServiceWatcher::serviceOwnerChanged,
            this, [this](const QString&, const QString&, const QString&) {
                qInfo() << "muoto-listener: installationhandler owner changed, re-subscribing";
                subscribeSession();
            });
}

void InstallListener::subscribeSession()
{
    if(!_session.isConnected())
    {
        qWarning() << "muoto-listener: session bus not connected";
        return;
    }

    _session.disconnect(QString::fromLatin1(kApkdService),
                      QString::fromLatin1(kApkdPath),
                      QString::fromLatin1(kApkdIface),
                      QStringLiteral("appInstalled"),
                      this,
                      SLOT(onApkdAppInstalled()));
    _session.disconnect(QString::fromLatin1(kApkdService),
                      QString::fromLatin1(kApkdPath),
                      QString::fromLatin1(kApkdIface),
                      QStringLiteral("appUpdated"),
                      this,
                      SLOT(onApkdAppUpdated()));
    _session.disconnect(QString::fromLatin1(kApkdService),
                      QString::fromLatin1(kApkdPath),
                      QString::fromLatin1(kApkdIface),
                      QStringLiteral("PropertiesChanged"),
                      this,
                      SLOT(onApkdPropertiesChanged(QString, QVariantMap, QStringList)));
    _session.disconnect(QString::fromLatin1(kInstService),
                      QString::fromLatin1(kInstPath),
                      QString::fromLatin1(kInstIface),
                      QStringLiteral("installFinished"),
                      this,
                      SLOT(onInstallationFinished(bool, QString)));

    _session.connect(QString::fromLatin1(kApkdService),
                     QString::fromLatin1(kApkdPath),
                     QString::fromLatin1(kApkdIface),
                     QStringLiteral("appInstalled"),
                     this,
                     SLOT(onApkdAppInstalled()));
    _session.connect(QString::fromLatin1(kApkdService),
                     QString::fromLatin1(kApkdPath),
                     QString::fromLatin1(kApkdIface),
                     QStringLiteral("appUpdated"),
                     this,
                     SLOT(onApkdAppUpdated()));
    _session.connect(QString::fromLatin1(kApkdService),
                     QString::fromLatin1(kApkdPath),
                     QString::fromLatin1(kApkdIface),
                     QStringLiteral("PropertiesChanged"),
                     this,
                     SLOT(onApkdPropertiesChanged(QString, QVariantMap, QStringList)));

    _session.connect(QString::fromLatin1(kInstService),
                     QString::fromLatin1(kInstPath),
                     QString::fromLatin1(kInstIface),
                     QStringLiteral("installFinished"),
                     this,
                     SLOT(onInstallationFinished(bool, QString)));
}

void InstallListener::subscribeSystem()
{
    if(!_system.isConnected())
    {
        qWarning() << "muoto-listener: system bus not connected";
        return;
    }

    _system.connect(QString::fromLatin1(kPkService),
                    QString::fromLatin1(kPkPath),
                    QString::fromLatin1(kPkIface),
                    QStringLiteral("TransactionListChanged"),
                    this,
                    SLOT(onPkTransactionListChanged(QStringList)));

    _system.connect(QString::fromLatin1(kLogin1Service),
                    QString::fromLatin1(kLogin1Path),
                    QString::fromLatin1(kLogin1Mgr),
                    QStringLiteral("PrepareForShutdown"),
                    this,
                    SLOT(onPrepareForShutdown(bool)));
}

void InstallListener::trackPkTransaction(const QString& path)
{
    if(path.isEmpty() || _pkTransactions.contains(path))
        return;

    _pkTransactions.insert(path);
    auto* watch = new PkTxWatch(path, this, this);
    _pkWatches.append(watch);

    _system.connect(QString::fromLatin1(kPkService),
                    path,
                    QString::fromLatin1(kPkTxIface),
                    QStringLiteral("Package"),
                    watch,
                    SLOT(onPackage(uint, QStringList, QString)));
    _system.connect(QString::fromLatin1(kPkService),
                    path,
                    QString::fromLatin1(kPkTxIface),
                    QStringLiteral("Finished"),
                    watch,
                    SLOT(onFinished(uint, uint)));
}

void InstallListener::onPkFinished(const QString& path, uint exitCode, bool relevant)
{
    _pkTransactions.remove(path);
    for(int i = 0; i < _pkWatches.size(); ++i)
    {
        if(_pkWatches.at(i)->path() == path)
        {
            _pkWatches.removeAt(i);
            break;
        }
    }

    if(exitCode == PK_EXIT_SUCCESS && relevant)
        scheduleApply("packagekit");
}

bool InstallListener::guardsBlockApply() const
{
    if(_shuttingDown)
        return true;
    if(QFile::exists(QString::fromLatin1(kOsUpdateSentinel)))
        return true;
    return false;
}

void InstallListener::scheduleApply(const char* trigger)
{
    if(guardsBlockApply())
    {
        qInfo() << "muoto-listener: apply skipped (guard):" << trigger;
        return;
    }
    _lastTrigger = trigger ? trigger : "unknown";
    qInfo() << "muoto-listener: scheduled apply, trigger" << _lastTrigger;
    _debounce.start();
}

void InstallListener::onApkdAppInstalled()
{
    scheduleApply("apkd-appInstalled");
}

void InstallListener::onApkdAppUpdated()
{
    scheduleApply("apkd-appUpdated");
}

void InstallListener::onApkdPropertiesChanged(const QString& interface,
                                                const QVariantMap& changed,
                                                const QStringList& invalidated)
{
    Q_UNUSED(invalidated);
    if(interface != QLatin1String(kApkdIface))
        return;
    const QVariant v = changed.value(QStringLiteral("containerReady"));
    if(v.isValid() && v.toBool())
        scheduleApply("apkd-containerReady");
}

void InstallListener::onInstallationFinished(bool success, const QString& errorString)
{
    Q_UNUSED(errorString);
    if(success)
        scheduleApply("installationhandler");
}

void InstallListener::onPkTransactionListChanged(const QStringList& transactions)
{
    for(const QString& path : transactions)
        trackPkTransaction(path);
}

void InstallListener::onPrepareForShutdown(bool active)
{
    if(active)
    {
        _shuttingDown = true;
        qInfo() << "muoto-listener: PrepareForShutdown, blocking applies";
    }
}

void InstallListener::onDebounceTimeout()
{
    if(guardsBlockApply())
    {
        qInfo() << "muoto-listener: debounce skipped (guard)";
        return;
    }
    if(_updateRunning)
    {
        qInfo() << "muoto-listener: debounce skipped (update running)";
        return;
    }
    qInfo() << "muoto-listener: debounce fired, trigger" << _lastTrigger;
    runUpdateScript();
}

void InstallListener::runUpdateScript()
{
    if(_updateRunning)
        return;

    qInfo() << "muoto-listener: starting" << kUpdateScript << "trigger" << _lastTrigger;

    auto* proc = new QProcess(this);
    _updateRunning = true;
    connect(proc,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            &InstallListener::onUpdateScriptFinished);
    proc->setProgram(QString::fromLatin1(kUpdateScript));
    proc->start();
    if(!proc->waitForStarted(5000))
    {
        qWarning() << "muoto-listener: failed to start" << kUpdateScript;
        _updateRunning = false;
        proc->deleteLater();
    }
}

void InstallListener::onUpdateScriptFinished(int exitCode, QProcess::ExitStatus status)
{
    _updateRunning = false;
    const char* statusStr = (status == QProcess::NormalExit) ? "normal" : "crashed";
    qInfo() << "muoto-listener: update-icons finished exit=" << exitCode
            << "status=" << statusStr << "trigger=" << _lastTrigger;
    if(exitCode != 0 || status != QProcess::NormalExit)
        qWarning() << "muoto-listener: update-icons failed";
    if(auto* proc = qobject_cast<QProcess*>(sender()))
        proc->deleteLater();
}

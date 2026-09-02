#ifndef INSTALLLISTENER_H
#define INSTALLLISTENER_H

#include <QObject>
#include <QDBusConnection>
#include <QHash>
#include <QProcess>
#include <QSet>
#include <QString>
#include <QTimer>

class InstallListener : public QObject
{
    Q_OBJECT
public:
    explicit InstallListener(QObject* parent = nullptr);
    void onPkFinished(const QString& path, uint exitCode, bool relevant);

private slots:
    void onApkdAppInstalled();
    void onApkdAppUpdated();
    void onInstallationFinished(bool success, const QString& errorString);
    void onPkTransactionListChanged(const QStringList& transactions);
    void onPrepareForShutdown(bool active);
    void onDebounceTimeout();
    void onUpdateScriptFinished(int exitCode, QProcess::ExitStatus status);

private:
    void scheduleApply(const char* trigger);
    void runUpdateScript();
    bool guardsBlockApply() const;
    void subscribeSession();
    void subscribeSystem();
    void syncPkTransactions();
    void trackPkTransaction(const QString& path);
    uint queryPkTransactionRole(const QString& path) const;

    void forgetPkTransactions();

    QTimer _debounce;
    // update-icons can legitimately run for minutes (two attempts, each waiting
    // on the icon lock). Without a bound, a hung one leaves _updateRunning set
    // forever and the listener silently stops re-theming until it is restarted.
    QTimer _updateWatchdog;
    QDBusConnection _session;
    QDBusConnection _system;
    bool _shuttingDown = false;
    bool _updateRunning = false;
    // A trigger that arrived while update-icons was running used to be dropped
    // outright, so the second of two back-to-back installs was never themed.
    bool _applyAgain = false;
    const char* _lastTrigger = "unknown";
    QProcess* _updateProc = nullptr;
    QSet<QString> _pkTransactions;
    QList<class PkTxWatch*> _pkWatches;
};

#endif

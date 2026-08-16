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

    QTimer _debounce;
    QDBusConnection _session;
    QDBusConnection _system;
    bool _shuttingDown = false;
    bool _updateRunning = false;
    const char* _lastTrigger = "unknown";
    QSet<QString> _pkTransactions;
    QList<class PkTxWatch*> _pkWatches;
};

#endif

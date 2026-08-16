#ifndef LAUNCHERSERVICE_H
#define LAUNCHERSERVICE_H

#include "iconjob.h"
#include "muotolauncherglobal.h"
#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QString>

class LauncherIconOps;

class MUOTO_LAUNCHER_EXPORT LauncherBackend : public QObject
{
    Q_OBJECT

public:
    explicit LauncherBackend(QObject* parent = nullptr);

    LauncherIconOps* iconOps() { return m_iconOps; }
    bool shuttingDown() const { return m_shuttingDown; }

signals:
    void prepareQuit();

private slots:
    void onPrepareForShutdown(bool active);

private:
    LauncherIconOps* m_iconOps;
    bool m_shuttingDown = false;
};

class MUOTO_LAUNCHER_EXPORT LauncherThemesAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.muoto.Launcher1.Themes")

public:
    explicit LauncherThemesAdaptor(LauncherBackend* backend, QObject* parent);

public slots:
    void ApplyIcons(const QString& pack, bool runPack, bool overlay,
                    const QDBusMessage& message);
    void RestoreIcons(const QDBusMessage& message);

signals:
    void OperationCompleted(const QString& op, bool ok, const QString& message);
    void Progress(const QString& op, int done, int total);

private:
    // Replies to the caller straight away, then hands the job to IconJobQueue.
    // The reply says "accepted", not "done": OperationCompleted follows when the
    // job actually runs, which may be after a drain.
    void enqueueOp(const QString& op, IconJob job, const QDBusMessage& message);

    LauncherBackend* m_backend;
    // Op of the job the queue is currently running, used to label Progress.
    // Derived from the running job rather than set around the call, which would
    // now always be clear by the time any progress arrived.
    QString m_runningOp;
};

#endif // LAUNCHERSERVICE_H

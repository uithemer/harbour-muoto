#ifndef LAUNCHERSERVICE_H
#define LAUNCHERSERVICE_H

#include "muotolauncherglobal.h"
#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>

#include <functional>

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
    void runIconOpVoid(const QString& op,
                       std::function<void(LauncherIconOps&)> start,
                       void (LauncherIconOps::*doneSignal)(bool, const QString&));

    LauncherBackend* m_backend;
};

#endif // LAUNCHERSERVICE_H

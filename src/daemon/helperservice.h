#ifndef HELPERSERVICE_H
#define HELPERSERVICE_H

#include <QObject>
#include <QTimer>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>

#include "densityenabler.h"

class HelperBackend : public QObject
{
    Q_OBJECT

public:
    explicit HelperBackend(QObject* parent = nullptr);

    DensityEnabler& densityEnabler() { return _densityEnabler; }

    void resetIdleTimer();
    void suspendIdleTimer();
    void resumeIdleTimer();
    bool shuttingDown() const { return _shuttingDown; }

signals:
    void idleQuit();

private slots:
    void onIdleTimeout();
    void onPrepareForShutdown(bool active);

private:
    DensityEnabler _densityEnabler;
    QTimer         _idleTimer;
    int            _idleSuspendCount = 0;
    bool           _shuttingDown = false;
};

class ThemesAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.muoto.Muoto1.Themes")

public:
    ThemesAdaptor(HelperBackend* backend, QObject* parent);

public slots:
    void DensityEnable(const QDBusMessage& message);

signals:
    void OperationCompleted(const QString& op, bool ok,
                            const QString& message);
    void Progress(const QString& op, int done, int total);

private:
    bool authorize(const QDBusMessage& message, const QString& op);
    HelperBackend* _backend;
};

class PacksAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.muoto.Muoto1.Packs")

public:
    PacksAdaptor(HelperBackend* backend, QObject* parent);

public slots:
    void UninstallPack(const QString& rpmName, const QDBusMessage& message);

signals:
    void OperationCompleted(const QString& op, bool ok,
                            const QString& message);

private:
    bool authorize(const QDBusMessage& message, const QString& op);
    HelperBackend* _backend;
};

#endif // HELPERSERVICE_H

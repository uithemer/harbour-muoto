#include "aliendalvikwatcher.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>

namespace {

const char* kApkdPath = "/com/jolla/apkd";
const char* kApkdIface = "com.jolla.apkd";

} // namespace

AlienDalvikWatcher::AlienDalvikWatcher(QObject* parent)
    : QObject(parent)
{
    // No sender filter: apkd takes a new bus name every time the container is
    // restarted, which is exactly the case this watcher exists for.
    if(!QDBusConnection::sessionBus().connect(QString(),
                                              QString::fromLatin1(kApkdPath),
                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("PropertiesChanged"),
                                              this,
                                              SLOT(handlePropertiesChanged(QString, QVariantMap, QStringList))))
    {
        qWarning() << "muoto-launcher: could not subscribe to apkd PropertiesChanged:"
                   << QDBusConnection::sessionBus().lastError().message();
    }
}

AlienDalvikWatcher* AlienDalvikWatcher::instance()
{
    static auto* watcher = new AlienDalvikWatcher;
    return watcher;
}

void AlienDalvikWatcher::handlePropertiesChanged(const QString& interface,
                                                 const QVariantMap& changed,
                                                 const QStringList& invalidated)
{
    Q_UNUSED(invalidated);

    if(interface != QLatin1String(kApkdIface))
        return;

    const QVariant ready = changed.value(QStringLiteral("containerReady"));
    if(!ready.isValid() || !ready.toBool())
        return;

    qInfo() << "muoto-launcher: apkd containerReady";
    emit containerReady();
}

#ifndef ALIENDALVIKWATCHER_H
#define ALIENDALVIKWATCHER_H

#include <QObject>
#include <QVariantMap>

// Tracks apkd's container readiness. Android support rewrites
// apkd_launcher_*.desktop while the container starts, roughly 17 s before it
// announces containerReady, so that signal is a safe point to re-theme the APK
// entries.
class AlienDalvikWatcher : public QObject
{
    Q_OBJECT

    explicit AlienDalvikWatcher(QObject* parent = nullptr);

public:
    static AlienDalvikWatcher* instance();

signals:
    void containerReady();

private slots:
    void handlePropertiesChanged(const QString& interface,
                                 const QVariantMap& changed,
                                 const QStringList& invalidated);
};

#endif // ALIENDALVIKWATCHER_H

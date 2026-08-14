#ifndef MCELPMSETTINGS_H
#define MCELPMSETTINGS_H

#include <QObject>
#include <QString>

class QQmlEngine;
class QJSEngine;

// Reads/writes Sailfish MCE low-power (Sneak Peek) settings over the
// system bus as defaultuser — no root, no mce-tools.
class MceLpmSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    static MceLpmSettings* instance();
    static QObject* qmlSingleton(QQmlEngine* engine, QJSEngine* scriptEngine);

    ~MceLpmSettings() override;

    bool enabled() const { return _enabled; }
    bool available() const { return _available; }

public slots:
    void refresh();
    // On: LPM + from-pocket|hover-over + ps-on-demand off.
    // Off: stock defaults (LPM off, from-pocket only, ps-on-demand on).
    bool applyProfile(bool on);

signals:
    void enabledChanged();
    void availableChanged();
    void error(const QString& message);

private:
    MceLpmSettings();

    bool getBool(const QString& key, bool* ok = nullptr);
    bool setBool(const QString& key, bool value);
    bool setInt(const QString& key, int value);

    bool _enabled;
    bool _available;
};

#endif

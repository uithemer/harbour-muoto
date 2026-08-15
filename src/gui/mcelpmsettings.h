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
    Q_PROPERTY(bool triggerFromPocket READ triggerFromPocket NOTIFY triggerFromPocketChanged)
    Q_PROPERTY(bool triggerHoverOver READ triggerHoverOver NOTIFY triggerHoverOverChanged)
    // true = proximity kept ready (MCE ps-on-demand disabled); needed for reliable glance.
    Q_PROPERTY(bool proximityReady READ proximityReady NOTIFY proximityReadyChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    static MceLpmSettings* instance();
    static QObject* qmlSingleton(QQmlEngine* engine, QJSEngine* scriptEngine);

    ~MceLpmSettings() override;

    bool enabled() const { return _enabled; }
    bool triggerFromPocket() const { return _triggerFromPocket; }
    bool triggerHoverOver() const { return _triggerHoverOver; }
    bool proximityReady() const { return _proximityReady; }
    bool available() const { return _available; }

public slots:
    void refresh();
    // Writes all three MCE keys from the given UI state.
    bool apply(bool enabled, bool fromPocket, bool hoverOver, bool proximityReady);
    // Stock defaults: LPM off, from-pocket only, proximity on-demand on.
    bool applyDefaults();

signals:
    void enabledChanged();
    void triggerFromPocketChanged();
    void triggerHoverOverChanged();
    void proximityReadyChanged();
    void availableChanged();
    void error(const QString& message);

private:
    MceLpmSettings();

    bool getBool(const QString& key, bool* ok = nullptr);
    int getInt(const QString& key, bool* ok = nullptr);
    bool setBool(const QString& key, bool value);
    bool setInt(const QString& key, int value);
    static int triggeringMask(bool fromPocket, bool hoverOver);

    bool _enabled;
    bool _triggerFromPocket;
    bool _triggerHoverOver;
    bool _proximityReady;
    bool _available;
};

#endif

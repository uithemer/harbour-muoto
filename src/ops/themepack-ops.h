#ifndef THEMEPACK_OPS_H
#define THEMEPACK_OPS_H

#include <QObject>
#include <QString>

// ThemePackOps: synchronous, root-only file/system tweaks formerly done
// by the small bash scripts under tps/ and scripts/. Lives in src/ops/
// so the daemon (and only the daemon) can drive them without pulling
// any GUI / QML / sailfishapp dependency.
//
// Each operation is intentionally cheap (a few systemctl calls plus
// a file write) and runs synchronously. The daemon serialises its bus
// dispatch on the Qt event loop, so a long-blocking call here would
// also block other D-Bus method handlers; if a future op needs more
// time, move it onto a QThread or a QProcess::startDetached pattern.
//
// All methods return bool so the daemon's adaptor can broadcast the
// (op, ok, message) tuple via OperationCompleted.
class ThemePackOps : public QObject
{
    Q_OBJECT

public:
    explicit ThemePackOps(QObject* parent = nullptr);

public slots:
    // Append "NoDisplay=true" to the legacy harbour-iconpacksupport
    // .desktop file so it stops showing up in the launcher. Idempotent
    // enough in practice (re-appending is harmless: the last NoDisplay=
    // wins); kept identical to the old setuid_ex(0) one-liner.
    bool hideIcon();

    // enabled == true  -> enable + start themepacksupport-autoupdate
    //                     {timer, service}
    // enabled == false -> stop + disable both
    // The GUI tracks the user-visible state in dconf; this slot only
    // owns the systemd side.
    bool setAutoupdate(bool enabled);

    // enabled == true  -> systemctl enable themepacksupport-systemupgrade
    // enabled == false -> systemctl disable themepacksupport-systemupgrade
    bool setServiceSu(bool enabled);

    // Rewrite /etc/systemd/system/themepacksupport-autoupdate.timer with
    // the requested cadence ("30", "1", "2", "3", "6", "12", or any
    // OnCalendar= expression for the wildcard branch). Mirrors the body
    // of the now-retired apply_hours.sh.
    bool applyHours(const QString& hours);

private:
    // Wrap QProcess::execute() with a friendly log + non-zero return
    // bubble-up. cmd is the bare program; args are the arguments.
    bool run(const QString& cmd, const QStringList& args);

    // Body of applyHours: produce the [Unit]/[Timer]/[Install] systemd
    // unit text matching the legacy script's case/esac.
    QString timerUnitText(const QString& hours) const;
};

#endif // THEMEPACK_OPS_H

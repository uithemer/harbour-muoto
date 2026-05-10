#include "themepack-ops.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QStringList>
#include <QSaveFile>
#include <QTextStream>
#include <QDebug>

namespace
{
    const char* kAutoupdateTimer = "themepacksupport-autoupdate.timer";
    const char* kAutoupdateService = "themepacksupport-autoupdate.service";
    const char* kSystemupgradeService = "themepacksupport-systemupgrade.service";

    const char* kIconPacksupportDesktop =
        "/usr/share/applications/harbour-iconpacksupport.desktop";

    const char* kAutoupdateTimerUnit =
        "/etc/systemd/system/themepacksupport-autoupdate.timer";
}

ThemePackOps::ThemePackOps(QObject* parent) : QObject(parent)
{
}

bool ThemePackOps::run(const QString& cmd, const QStringList& args)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(cmd, args);
    if(!p.waitForStarted(5000))
    {
        qWarning() << "ThemePackOps: failed to start" << cmd << args;
        return false;
    }
    if(!p.waitForFinished(20000))
    {
        qWarning() << "ThemePackOps: timeout" << cmd << args;
        p.kill();
        return false;
    }
    if(p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
    {
        qWarning() << "ThemePackOps:" << cmd << args
                   << "exited" << p.exitCode();
        return false;
    }
    return true;
}

bool ThemePackOps::hideIcon()
{
    // Append the NoDisplay=true line. Note: re-running adds another
    // copy, but the .desktop spec says the last value wins, so it stays
    // hidden either way. Matches the old `echo >> ... .desktop` shell
    // one-liner verbatim.
    QFile f(QString::fromLatin1(kIconPacksupportDesktop));
    if(!f.exists())
    {
        // Nothing to hide; treat as success so the GUI's busy spinner
        // clears.
        return true;
    }
    if(!f.open(QIODevice::Append | QIODevice::Text))
    {
        qWarning() << "ThemePackOps::hideIcon: cannot open"
                   << kIconPacksupportDesktop << f.errorString();
        return false;
    }
    QTextStream ts(&f);
    ts << "NoDisplay=true\n";
    return true;
}

bool ThemePackOps::setAutoupdate(bool enabled)
{
    // The GUI owns the autoUpdate dconf key; this just toggles the
    // backing systemd units. (The retired tps/ shell mirror also wrote
    // autoupd=1/0 into config.cfg, but no consumer of that file
    // survives.)
    bool ok = true;
    const QString verb = enabled ? QStringLiteral("enable")
                                 : QStringLiteral("disable");
    const QString verb2 = enabled ? QStringLiteral("start")
                                  : QStringLiteral("stop");
    ok &= run(QStringLiteral("systemctl"),
              QStringList() << verb << kAutoupdateTimer);
    ok &= run(QStringLiteral("systemctl"),
              QStringList() << verb2 << kAutoupdateTimer);
    ok &= run(QStringLiteral("systemctl"),
              QStringList() << verb << kAutoupdateService);
    ok &= run(QStringLiteral("systemctl"),
              QStringList() << verb2 << kAutoupdateService);
    return ok;
}

bool ThemePackOps::setServiceSu(bool enabled)
{
    if(enabled)
    {
        return run(QStringLiteral("systemctl"),
                   QStringList() << QStringLiteral("enable")
                                 << kSystemupgradeService);
    }
    return run(QStringLiteral("systemctl"),
               QStringList() << QStringLiteral("disable")
                             << kSystemupgradeService);
}

QString ThemePackOps::timerUnitText(const QString& hours) const
{
    // Mirrors the case/esac in the retired tps/apply_hours.sh:
    //   "30" / "1" / "2" / "3" / "6" / "12" map to the matching
    //   OnCalendar/OnActiveSec pair; anything else lands in the
    //   wildcard branch with OnCalendar=<raw> and OnActiveSec=24h.
    QString onCalendar;
    QString onActive;
    if(hours == QLatin1String("30"))
    {
        onCalendar = QStringLiteral("*-*-* *:0/30");
        onActive   = QStringLiteral("30m");
    }
    else if(hours == QLatin1String("1"))
    {
        onCalendar = QStringLiteral("hourly");
        onActive   = QStringLiteral("1h");
    }
    else if(hours == QLatin1String("2"))
    {
        onCalendar = QStringLiteral("2h");
        onActive   = QStringLiteral("2h");
    }
    else if(hours == QLatin1String("3"))
    {
        onCalendar = QStringLiteral("3h");
        onActive   = QStringLiteral("3h");
    }
    else if(hours == QLatin1String("6"))
    {
        onCalendar = QStringLiteral("6h");
        onActive   = QStringLiteral("6h");
    }
    else if(hours == QLatin1String("12"))
    {
        onCalendar = QStringLiteral("12h");
        onActive   = QStringLiteral("12h");
    }
    else
    {
        onCalendar = hours;
        onActive   = QStringLiteral("24h");
    }

    QString unit;
    unit.reserve(256);
    unit += QStringLiteral("[Unit]\n");
    unit += QStringLiteral("Description=Timer for updating icon theme via Theme pack support.\n");
    unit += QStringLiteral("[Timer]\n");
    unit += QStringLiteral("OnBootSec=0\n");
    unit += QStringLiteral("OnCalendar=") + onCalendar + QLatin1Char('\n');
    unit += QStringLiteral("Persistent=true\n");
    unit += QStringLiteral("OnActiveSec=") + onActive + QLatin1Char('\n');
    unit += QLatin1Char('\n');
    unit += QStringLiteral("[Install]\n");
    unit += QStringLiteral("WantedBy=timers.target\n");
    return unit;
}

bool ThemePackOps::applyHours(const QString& hours)
{
    // Persist the human-readable cadence under service/hours so the GUI
    // can display the current value (matches the old script).
    {
        const QString hoursPath =
            QStringLiteral("/usr/share/sailfishos-uithemer/service/hours");
        QDir().mkpath(QFileInfo(hoursPath).absolutePath());
        QSaveFile h(hoursPath);
        if(h.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            QTextStream ts(&h);
            ts << hours << '\n';
            ts.flush();
            h.commit();
        }
        else
        {
            qWarning() << "ThemePackOps::applyHours: cannot open" << hoursPath
                       << h.errorString();
        }
    }

    // Rewrite the timer unit in /etc/systemd/system/.
    QSaveFile f(QString::fromLatin1(kAutoupdateTimerUnit));
    if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        qWarning() << "ThemePackOps::applyHours: cannot open"
                   << kAutoupdateTimerUnit << f.errorString();
        return false;
    }
    {
        QTextStream ts(&f);
        ts << timerUnitText(hours);
        ts.flush();
    }
    if(!f.commit())
    {
        qWarning() << "ThemePackOps::applyHours: commit failed"
                   << kAutoupdateTimerUnit << f.errorString();
        return false;
    }

    // applyHours is only ever invoked while autoUpdate is on (the GUI
    // hides the cadence menu otherwise), so we just need systemd to
    // reread the rewritten unit and bounce the timer.
    bool ok = run(QStringLiteral("systemctl"),
                  QStringList() << QStringLiteral("daemon-reload"));
    ok &= run(QStringLiteral("systemctl"),
              QStringList() << QStringLiteral("restart") << kAutoupdateTimer);
    return ok;
}

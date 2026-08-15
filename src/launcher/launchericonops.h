#ifndef LAUNCHERICONOPS_H
#define LAUNCHERICONOPS_H

#include "muotolauncherglobal.h"
#include <QObject>
#include <QString>

class MUOTO_LAUNCHER_EXPORT LauncherIconOps : public QObject
{
    Q_OBJECT

public:
    static LauncherIconOps* instance();

    void applyIcons(const QString& pack, bool runPack, bool overlay);
    void restoreIcons();

    bool applyPackIcons() const { return m_applyPackIcons; }

    bool restoreOnUpdaterDestroy() const { return m_restoreOnUpdaterDestroy; }

    void rebuildIconUpdaters();

signals:
    void progress(int done, int total);
    void applied(bool ok, const QString& message);
    void restored(bool ok, const QString& message);

private:
    explicit LauncherIconOps(QObject* parent = nullptr);

    void rebuildIconUpdatersNow();
    void clearUpdaters(bool restoreOnDestroy);
    void reloadIconPacks();
    void ensureDesktopWatches();
    void rearmApkDesktopWatches();

    // Silent unless an apply/restore is in flight: rebuildIconUpdatersNow()
    // also runs from dconf watches, where there is nothing to report.
    void emitProgress(int done, int total);

    bool m_restoreOnUpdaterDestroy = true;
    bool m_applyPackIcons = true;
    bool m_inIconOp = false;
    bool m_rebuilding = false;
    // Desktop entries plus one step for the folder-tile pass that follows.
    int m_progressTotal = 0;
    // Last reported percentage, used to rate-limit the progress signal.
    int m_progressPercent = -1;
};

#endif // LAUNCHERICONOPS_H

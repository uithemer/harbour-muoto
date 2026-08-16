#ifndef LAUNCHERICONOPS_H
#define LAUNCHERICONOPS_H

#include "muotolauncherglobal.h"
#include <QObject>
#include <QString>
#include <QTimer>

class QFileSystemWatcher;

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
    // Drops generated PNGs nothing references; keeps the ones a desktop or a
    // manifest entry still names.
    void reconcileGeneratedIcons();
    void clearUpdaters(bool restoreOnDestroy);
    void reloadIconPacks();
    void ensureDesktopWatches();
    void rearmApkDesktopWatches();
    void rearmAllDesktopWatches();

    // Re-theme only the APK bridge entries after apkd regenerated them.
    void refreshApkIcons(bool scheduleVerify);
    bool apkIconsClobbered() const;

    // Theme apps installed or updated behind our back, whatever installer did it.
    void ensureDesktopDirWatch();
    void refreshNewDesktops();
    QStringList desktopsNeedingTheme() const;

    // Silent unless an apply/restore is in flight: rebuildIconUpdatersNow()
    // also runs from dconf watches, where there is nothing to report.
    void emitProgress(int done, int total);

    QFileSystemWatcher* m_desktopDirWatcher = nullptr;
    QTimer m_desktopScan;

    // Reset per rebuild. Apply reports ok / partial / hard failure from these
    // instead of assuming success, so a device that themed nothing (a lost
    // cap_dac_override, say) stops toasting "Theme updated."
    int m_updatersBuilt = 0;
    int m_updatersWritten = 0;

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

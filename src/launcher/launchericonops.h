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

    void clearUpdaters(bool restoreOnDestroy);
    void reloadIconPacks();
    void ensureDesktopWatches();

    bool m_restoreOnUpdaterDestroy = true;
    bool m_applyPackIcons = true;
};

#endif // LAUNCHERICONOPS_H

#ifndef THEMEPACKMODEL_H
#define THEMEPACKMODEL_H
#include <QAbstractListModel>
#include <QFile>
#include <QDir>
#include <QStringList>
#include "fontapplier.h"
#include "densityenabler.h"

class HelperClient;

// ThemePackModel: list-model of installed harbour-themepack-* packs +
// orchestration used by Icons/Fonts configure pages and ThemeWork.
//
// 2.7.0: the OCR (one-click-restore) state machine and the Recovery
// dialog are retired together with OptionsPage; the surface here is
// now just apply/restore/uninstall/density and the read-only
// list/role queries. Privileged work (icon writes, rpm -e) is
// forwarded to the helper daemon via HelperClient; everything else
// runs in-process as defaultuser.
class ThemePackModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        enum ThemePackRoles { PackDisplayNameRole = Qt::UserRole, PackNameRole };

    public:
        explicit ThemePackModel(QObject *parent = 0);

    public slots:
        QString readThemePackName(const QString& packname) const;
        bool hasCapability(int index, const QString& capability) const;

    public slots:
        void applyTheme(int index, bool font, const QString& weight);
        void restoreTheme(bool font);
        void uninstall(int index);
        void restoreDpi(bool dpr, bool iconSize);

    public slots:
        QString packName(int index) const;
        QString packDisplayName(int index) const;
        bool hasIcons(int index) const;
        bool hasNative(int index) const;
        bool hasJolla(int index) const;
        bool hasApk(int index) const;
        bool hasIconOverlay(int index) const;
        bool hasDynClock(int index) const;
        bool hasDynCalendar(int index) const;
        bool hasDynClockForPack(const QString& packName) const;
        bool hasDynCalendarForPack(const QString& packName) const;
        bool hasFont(int index) const;
        bool hasFontNonLatin(int index) const;
        void reloadAll();

    public:
        virtual QHash<int, QByteArray> roleNames() const;
        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual int rowCount(const QModelIndex &) const;

    signals:
        void themeApplied();
        void themeApplyFailed(const QString& message);
        void themeRestored();
        void themeRestoreFailed(const QString& message);
        void uninstallCompleted();
        void uninstallFailed(const QString& message);
        void dpiRestored();
        void dpiRestoreFailed(const QString& message);

    private:
        // Look up the rpm (package) name that owns a given pack
        // directory. Used by uninstall() to feed the daemon's
        // Packs.UninstallPack(s rpmName).
        QString rpmNameForPack(const QString& packDirName) const;

        QStringList _packlist;
        QStringList _packnames;
        FontApplier _fonts;
        DensityEnabler _density;
        HelperClient* _helper;
};

#endif // THEMEPACKMODEL_H

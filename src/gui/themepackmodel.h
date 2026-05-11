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
// the orchestration glue used by MainPage / OptionsPage / DensityPage.
//
// 2.6.0 split:
//   - In-process (defaultuser): applyTheme / restoreTheme / recoveryTheme
//     (font-only steps), restoreDpi (dconf reset), reloadAll, list/role
//     queries. These never touch root.
//   - Through HelperClient (system-bus + polkit): uninstall (rpm -e),
//     and the icon legs that ocr() chains together.
//
// ocr() is a pure GUI-side state machine:
//   Helper.setAutoupdate(false)     [System-bus, manage-system-services]
//   -> Helper.restoreIcons()        [System-bus, manage-themes]
//   -> _fonts.restoreFonts()        [in-process]
//   -> _density.restoreDensity(t,t) [in-process]
//   -> dconf writes to              [in-process; defaultuser owns]
//        /desktop/lipstick/sailfishos-uithemer/{activeIconPack,
//                                               activeFontPack,
//                                               autoUpdate}
//   -> emit ocrRestored()
// No OneClickRestore D-Bus method exists; the daemon never sees the
// orchestration logic.
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
        void ocr();
        void recoveryTheme(bool font);

    public slots:
        QString packName(int index) const;
        QString packDisplayName(int index) const;
        bool hasIcons(int index) const;
        bool hasNative(int index) const;
        bool hasApk(int index) const;
        bool hasIconOverlay(int index) const;
        bool hasFont(int index) const;
        bool hasFontNonLatin(int index) const;
        void reloadAll();

    public:
        virtual QHash<int, QByteArray> roleNames() const;
        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual int rowCount(const QModelIndex &) const;

    signals:
        void themeApplied();
        void themeRestored();
        void uninstallCompleted();
        void dpiRestored();
        void ocrRestored();
        void themeRecovered();

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

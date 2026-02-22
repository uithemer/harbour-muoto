#ifndef THEMEPACKMODEL_H
#define THEMEPACKMODEL_H
#include <QAbstractListModel>
#include <QFile>
#include <QDir>
#include <QStringList>

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
        void applyTheme(int index, bool icon, bool overlay, bool font, const QString& weight, bool sound);
        void iconsPreview(int index);
        void restoreTheme(bool icon, bool font, bool sound);
        void uninstall(int index);
        void reapplyIcons();
        void applyADPI(const QString& adpi);
        void restoreDpi(bool dpr, bool adpi);
        void ocr();
        void recoveryTheme(bool icon, bool font, bool sound);
        void toolsBackupIcons();
        void toolsRestoreIcons(const QString& filename);

    public slots:
        QString packName(int index) const;
        QString packDisplayName(int index) const;
        bool hasIcons(int index) const;
        bool hasJolla(int index) const;
        bool hasNative(int index) const;
        bool hasApk(int index) const;
        bool hasIconOverlay(int index) const;
        bool hasFont(int index) const;
        bool hasSound(int index) const;
        bool hasFontNonLatin(int index) const;
        bool hasDynClock(int index) const;
        bool hasDynCal(int index) const;
        void reloadAll();

    public:
        virtual QHash<int, QByteArray> roleNames() const;
        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual int rowCount(const QModelIndex &) const;

    signals:
        void themeApplied();
        void iconReapplied();
        void iconsPreviewed();
        void themeRestored();
        void uninstallCompleted();
        void dpiRestored();
        void ocrRestored();
        void themeRecovered();
        void toolsApplied();

    private:
        QStringList _packlist;
        QStringList _packnames;
};

#endif // THEMEPACKMODEL_H

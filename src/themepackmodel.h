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
        void applyTheme(int index, bool font, const QString& weight, bool sound);
        void restoreTheme(bool font, bool sound);
        void uninstall(int index);
        void applyADPI(const QString& adpi);
        void restoreDpi(bool dpr, bool adpi);
        void ocr();
        void recoveryTheme(bool font, bool sound);

    public slots:
        QString packName(int index) const;
        QString packDisplayName(int index) const;
        bool hasIcons(int index) const;
        bool hasNative(int index) const;
        bool hasApk(int index) const;
        bool hasIconOverlay(int index) const;
        bool hasFont(int index) const;
        bool hasSound(int index) const;
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
        QStringList _packlist;
        QStringList _packnames;
};

#endif // THEMEPACKMODEL_H

#ifndef FONTAPPLIER_H
#define FONTAPPLIER_H

#include <QObject>
#include <QString>

// FontApplier: per-user font theming via fontconfig aliasing.
//
// Replaces the legacy approach (overwriting /usr/share/fonts/sail-sans-pro
// and friends from bash scripts) with the supported Sailfish mechanism:
// write a single fontconfig conf file under
//   /home/defaultuser/.config/fontconfig/conf.d/99-uithemer.conf
// that re-aliases "Sail Sans Pro Light" / "Sail Sans Pro" to the family
// shipped by the user-selected theme pack, then run fc-cache -f as
// defaultuser. Restore == remove the conf file + fc-cache.
//
// Aliendalvik (/opt/alien/system/fonts) is not handled here: the Android
// runtime ignores fontconfig.
//
// All operations serialise on the same FileLock sentinel as IconApplier
// (/usr/share/sailfishos-uithemer/icon-backup.lock) so font / icon jobs
// never race each other.
class FontApplier : public QObject
{
    Q_OBJECT

public:
    explicit FontApplier(QObject* parent = nullptr);

public slots:
    // Write the fontconfig conf for `packName` (bare or full
    // "harbour-themepack-..." form), aliasing Sail Sans Pro to the family
    // exposed by `<pack>/font/<weightBasename>.ttf`. Refreshes fc-cache.
    void applyFromPack(const QString& packName, const QString& weightBasename);

    // Remove the conf file (no-op if missing) and refresh fc-cache.
    void restoreFonts();

    // Symmetric API for OptionsPage's recovery flow. Identical to
    // restoreFonts(): with the fontconfig approach there is nothing to
    // reinstall (no system files were ever overwritten), so "recovery"
    // collapses to "restore".
    void recoveryFonts();

signals:
    void applied(const QString& packName);
    void restored();
    void error(const QString& message);

private:
    QString packDir(const QString& packName) const;
    QString familyFromTtf(const QString& path) const;
    QString buildConfXml(const QString& packName,
                        const QString& weightBasename,
                        const QString& latinFamily) const;
    bool writeConf(const QString& xml);
    bool removeConf();
    void runFcCache();
    void chownToDefaultUser(const QString& path) const;

    static const char* kConfPath;
    static const char* kConfDir;
};

#endif // FONTAPPLIER_H

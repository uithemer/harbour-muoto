#ifndef FONTAPPLIER_H
#define FONTAPPLIER_H

#include <QObject>
#include <QString>

// FontApplier: per-user font theming via fontconfig aliasing.
//
// Replaces the legacy approach (overwriting /usr/share/fonts/sail-sans-pro
// and friends from bash scripts) with the supported Sailfish mechanism:
// write a single fontconfig conf file under
//   ~/.config/fontconfig/conf.d/99-muoto.conf
// that re-aliases "Sail Sans Pro Light" / "Sail Sans Pro" to the family
// shipped by the user-selected theme pack, then run fc-cache -f.
//
// 3.2.2: pack TTFs are copied into ~/.local/share/fonts/muoto/ (Sailjail-
// readable) and the conf <dir> entries point there — not at
// /usr/share/harbour-themepack-* or ~/.themepack, which jailed apps cannot
// open. Restore removes the conf and the staging tree.
//
// 2.6.0: this class is intentionally GUI-only (lives in src/gui/, never
// exposed via the helperd D-Bus surface) because it is fully
// unprivileged. The GUI process itself runs as defaultuser, so writing
// the conf to the current user's $HOME and running `fc-cache -f` as
// the current user is exactly what the legacy `su - defaultuser` shells
// were emulating from the old setuid GUI. No daemon round-trip needed.
//
// Aliendalvik (/opt/alien/system/fonts) is not handled here: the Android
// runtime ignores fontconfig.
//
// All operations use the same non-blocking FileLock sentinel as icon ops
// (/usr/share/harbour-muoto/icon-ops.lock) so font / icon jobs
// never race each other.
class FontApplier : public QObject
{
    Q_OBJECT

public:
    explicit FontApplier(QObject* parent = nullptr);

public slots:
    // Stage pack fonts under ~/.local/share/fonts/muoto/, write the
    // fontconfig conf for `packName` (bare or full "harbour-themepack-..."
    // form), aliasing Sail Sans Pro to the family exposed by
    // `<pack>/font/<weightBasename>.ttf`. Refreshes fc-cache.
    void applyFromPack(const QString& packName, const QString& weightBasename);

    // Remove the conf file and staging tree (no-op if missing) and refresh
    // fc-cache.
    void restoreFonts();

signals:
    void applied(const QString& packName);
    void restored();
    void error(const QString& message);

private:
    QString packDir(const QString& packName) const;
    QString stageRoot() const;
    QString stageFontDir() const;
    QString stageNonlatinDir() const;
    QString familyFromTtf(const QString& path) const;
    QString buildConfXml(const QString& packName,
                        const QString& weightBasename,
                        const QString& latinFamily) const;
    bool clearStage() const;
    bool copyDirFiles(const QString& srcDir, const QString& dstDir) const;
    bool stagePackFonts(const QString& packName) const;
    bool writeConf(const QString& xml);
    bool removeConf();
    void runFcCache();
    QString confDir() const;
    QString confPath() const;
};

#endif // FONTAPPLIER_H

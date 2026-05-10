#include "themepackmodel.h"
#include "helperclient.h"

#include <QProcess>
#include <QStringList>
#include <QMetaObject>
#include <algorithm>
#include <QDebug>

#define RAW_PACK_NAME(x) QString(x).replace("harbour-themepack-", "")

ThemePackModel::ThemePackModel(QObject *parent)
    : QAbstractListModel(parent)
    , _helper(new HelperClient(this))
{
    // Errors are best-effort logged; the busy spinner in QML stays on
    // until applied/restored/recovered actually fires.
    QObject::connect(&_fonts, &FontApplier::error,
                     this, [](const QString& m) { qWarning() << "FontApplier error:" << m; });
    QObject::connect(&_density, &DensityEnabler::error,
                     this, [](const QString& m) { qWarning() << "DensityEnabler error:" << m; });
    QObject::connect(_helper, &HelperClient::error, this,
                     [](const QString& op, const QString& m) {
        qWarning() << "HelperClient error" << op << ":" << m;
    });

    // packUninstalled fires from the daemon's Packs.OperationCompleted
    // when rpm -e succeeded; relay to QML's existing
    // ThemePackModel::uninstallCompleted signal (the QML side does
    // model invalidation in onUninstallCompleted handlers, so we also
    // mutate _packlist / _packnames in the slot before emitting).
    QObject::connect(_helper, &HelperClient::packUninstalled, this,
                     [this](const QString& rpmName) {
        Q_UNUSED(rpmName);
        // Refresh the list-model wholesale: cheap, and avoids racing
        // with the daemon's per-row index that we no longer track.
        beginResetModel();
        QDir dir("/usr/share");
        dir.setNameFilters(QDir::nameFiltersFromString("harbour-themepack-*"));
        const QStringList entries = dir.entryList(QDir::Dirs, QDir::Name);
        _packlist.clear();
        _packnames.clear();
        for(const QString& packname : entries)
        {
            _packlist << packname;
            _packnames << this->readThemePackName(packname);
        }
        endResetModel();
        emit uninstallCompleted();
    });

    this->reloadAll();
}

QString ThemePackModel::readThemePackName(const QString &packname) const
{
    QFile file(QStringLiteral("/usr/share/") + packname + QStringLiteral("/package"));
    if(!file.open(QFile::ReadOnly))
        return QString();
    const QString s = QString::fromUtf8(file.readAll()).simplified();
    file.close();
    return s;
}

bool ThemePackModel::hasCapability(int index, const QString &capability) const
{
    if(this->_packnames.empty())
        return false;

    QDir dir("/usr/share/" + this->_packlist[index] + "/" + capability);

    if(!dir.exists())
        return false;

    return dir.count() > 0;
}

void ThemePackModel::applyTheme(int index, bool font, const QString& weight)
{
    if(!font)
    {
        emit themeApplied();
        return;
    }
    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = QObject::connect(&_fonts, &FontApplier::applied, this,
                             [this, conn](const QString&) {
        emit themeApplied();
        QObject::disconnect(*conn);
        delete conn;
    });
    _fonts.applyFromPack(RAW_PACK_NAME(this->_packlist[index]), weight);
}

void ThemePackModel::restoreTheme(bool font)
{
    if(!font)
    {
        emit themeRestored();
        return;
    }
    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = QObject::connect(&_fonts, &FontApplier::restored, this,
                             [this, conn]() {
        emit themeRestored();
        QObject::disconnect(*conn);
        delete conn;
    });
    _fonts.restoreFonts();
}

void ThemePackModel::restoreDpi(bool dpr, bool iconSize)
{
    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = QObject::connect(&_density, &DensityEnabler::restored, this,
                             [this, conn]() {
        emit dpiRestored();
        QObject::disconnect(*conn);
        delete conn;
    });
    _density.restoreDensity(dpr, iconSize);
}

void ThemePackModel::ocr()
{
    // 2.6.0: orchestrated locally so most of the steps stay in the
    // unprivileged GUI process. Two daemon calls (SetAutoupdate,
    // RestoreIcons) and three in-process steps (font restore, density
    // restore, dconf writes). Each step is chained by a single-shot
    // lambda connect on the matching success signal; on any failure
    // helper.error -> we emit ocrRestored() so the spinner clears.

    auto* errBail = new QMetaObject::Connection;
    *errBail = QObject::connect(_helper, &HelperClient::error, this,
                                [this, errBail](const QString& op,
                                                const QString& msg) {
        qWarning() << "ocr aborted at" << op << ":" << msg;
        QObject::disconnect(*errBail);
        delete errBail;
        emit ocrRestored();
    });

    auto disconnectErrBail = [errBail]() {
        QObject::disconnect(*errBail);
        delete errBail;
    };

    auto runDconf = [](const QString& key, const QString& value) {
        // QML side normally drives ConfigurationGroup writes, but the
        // ocr flow runs entirely in C++ here so we shell out to
        // `dconf write`. The GUI is defaultuser so no `su -` glue is
        // needed -- the writes land in the right per-user db.
        QProcess::execute(QStringLiteral("dconf"),
                          QStringList() << QStringLiteral("write")
                                        << key << value);
    };

    auto* c1 = new QMetaObject::Connection;
    *c1 = QObject::connect(_helper, &HelperClient::serviceChanged, this,
                           [this, c1, runDconf, disconnectErrBail]() {
        QObject::disconnect(*c1);
        delete c1;

        // Step 2: ask daemon to RestoreIcons.
        auto* c2 = new QMetaObject::Connection;
        *c2 = QObject::connect(_helper, &HelperClient::iconsRestored, this,
                               [this, c2, runDconf, disconnectErrBail]() {
            QObject::disconnect(*c2);
            delete c2;

            // Step 3: in-process font restore.
            auto* c3 = new QMetaObject::Connection;
            *c3 = QObject::connect(&_fonts, &FontApplier::restored, this,
                                   [this, c3, runDconf, disconnectErrBail]() {
                QObject::disconnect(*c3);
                delete c3;

                // Step 4: in-process density restore (DPR + icon size).
                auto* c4 = new QMetaObject::Connection;
                *c4 = QObject::connect(&_density, &DensityEnabler::restored,
                                       this,
                                       [this, c4, runDconf,
                                        disconnectErrBail]() {
                    QObject::disconnect(*c4);
                    delete c4;

                    // Step 5: dconf writes for the GUI-owned
                    // bookkeeping keys, mirroring what tps/ocr.sh used
                    // to do with `dconf write` shells.
                    runDconf(QStringLiteral("/desktop/lipstick/sailfishos-uithemer/activeIconPack"),
                             QStringLiteral("'default'"));
                    runDconf(QStringLiteral("/desktop/lipstick/sailfishos-uithemer/activeFontPack"),
                             QStringLiteral("'default'"));
                    runDconf(QStringLiteral("/desktop/lipstick/sailfishos-uithemer/autoUpdate"),
                             QStringLiteral("0"));

                    disconnectErrBail();
                    emit ocrRestored();
                });
                _density.restoreDensity(true, true);
            });
            _fonts.restoreFonts();
        });
        _helper->restoreIcons();
    });

    _helper->setAutoupdate(false);
}

void ThemePackModel::recoveryTheme(bool font)
{
    if(!font)
    {
        emit themeRecovered();
        return;
    }
    // FontApplier::recoveryFonts() emits restored(); map that to
    // themeRecovered() (not themeRestored, otherwise MainPage's
    // onThemeRestored handler would also fire while OptionsPage is on
    // top of the page stack).
    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = QObject::connect(&_fonts, &FontApplier::restored, this,
                             [this, conn]() {
        emit themeRecovered();
        QObject::disconnect(*conn);
        delete conn;
    });
    _fonts.recoveryFonts();
}

QString ThemePackModel::rpmNameForPack(const QString& packDirName) const
{
    // Replaces the old `Spawner::executeSync("rpm -qf ... %{NAME}\\n")`
    // call: we just need the RPM-package owner of the pack's /usr/share
    // directory so the daemon can `rpm -e <name>`.
    QProcess p;
    p.start(QStringLiteral("rpm"),
            QStringList()
                << QStringLiteral("-qf")
                << (QStringLiteral("/usr/share/") + packDirName + QStringLiteral("/"))
                << QStringLiteral("--queryformat") << QStringLiteral("%{NAME}\n"));
    if(!p.waitForFinished(10000))
    {
        qWarning() << "rpmNameForPack: rpm timed out for" << packDirName;
        return QString();
    }
    const QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    return out;
}

void ThemePackModel::uninstall(int index)
{
    if(index < 0 || index >= _packlist.size())
        return;

    const QString rpmName = rpmNameForPack(_packlist[index]);
    if(rpmName.isEmpty())
    {
        qWarning() << "uninstall: cannot determine rpm for"
                   << _packlist[index];
        emit uninstallCompleted();
        return;
    }
    // Per-row removal happens in the HelperClient::packUninstalled
    // lambda set up in the ctor (we beginResetModel/endResetModel
    // there to cover any concurrent change to /usr/share/).
    _helper->uninstallPack(rpmName);
}

QString ThemePackModel::packName(int index) const
{
    if(this->_packlist.empty())
        return QString();

    return this->_packlist[index];
}

QString ThemePackModel::packDisplayName(int index) const
{
    if(this->_packnames.empty())
        return QString();

    return this->_packnames[index];
}

bool ThemePackModel::hasIcons(int index) const
{
    return this->hasNative(index) || this->hasApk(index) || this->hasIconOverlay(index);
}

bool ThemePackModel::hasNative(int index) const
{
    return this->hasCapability(index, "native");
}

bool ThemePackModel::hasApk(int index) const
{
    return this->hasCapability(index, "apk");
}

bool ThemePackModel::hasIconOverlay(int index) const
{
    return this->hasCapability(index, "overlay");
}

bool ThemePackModel::hasFont(int index) const
{
    return this->hasCapability(index, "font");
}

bool ThemePackModel::hasFontNonLatin(int index) const
{
    return this->hasCapability(index, "font-nonlatin");
}

void ThemePackModel::reloadAll()
{
    QDir dir("/usr/share");
    dir.setNameFilters(QDir::nameFiltersFromString("harbour-themepack-*"));

    this->beginResetModel();
    QStringList entries = dir.entryList(QDir::Dirs, QDir::Name);

    this->_packlist.clear();
    this->_packnames.clear();

    std::for_each(entries.begin(), entries.end(), [this](const QString& packname) {
        this->_packlist << packname;
        this->_packnames << this->readThemePackName(packname);
    });

    this->endResetModel();
}

QHash<int, QByteArray> ThemePackModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[ThemePackRoles::PackDisplayNameRole] = "packDisplayName";
    roles[ThemePackRoles::PackNameRole] = "packName";
    return roles;
}

QVariant ThemePackModel::data(const QModelIndex &index, int role) const
{
    if(role == ThemePackRoles::PackDisplayNameRole)
        return this->_packnames[index.row()];

    if(role == ThemePackRoles::PackNameRole)
        return RAW_PACK_NAME(this->_packlist[index.row()]);

    return QVariant();
}

int ThemePackModel::rowCount(const QModelIndex &) const
{
    return this->_packlist.count();
}

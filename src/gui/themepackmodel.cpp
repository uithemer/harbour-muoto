#include "themepackmodel.h"
#include "helperclient.h"
#include "iconpaths.h"

#include <QProcess>
#include <QStringList>
#include <QMetaObject>
#include <algorithm>
#include <QDebug>

#define RAW_PACK_NAME(x) QString(x).replace("harbour-themepack-", "")

ThemePackModel::ThemePackModel(QObject *parent)
    : QAbstractListModel(parent)
    , _helper(HelperClient::instance())
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
    if(index < 0 || index >= _packlist.size())
        return false;

    return IconPaths::packCapabilityUsable(QStringLiteral("/usr/share/") + _packlist.at(index),
                                           capability);
}

void ThemePackModel::applyTheme(int index, bool font, const QString& weight)
{
    if(!font)
    {
        emit themeApplied();
        return;
    }
    auto* failed = new bool(false);
    QMetaObject::Connection* errConn = new QMetaObject::Connection;
    QMetaObject::Connection* okConn = new QMetaObject::Connection;
    *errConn = QObject::connect(&_fonts, &FontApplier::error, this,
                                [this, failed, errConn, okConn](const QString& message) {
        *failed = true;
        emit themeApplyFailed(message);
        QObject::disconnect(*errConn);
        QObject::disconnect(*okConn);
        delete errConn;
        delete okConn;
        delete failed;
    });
    *okConn = QObject::connect(&_fonts, &FontApplier::applied, this,
                               [this, failed, errConn, okConn](const QString&) {
        if(!*failed)
            emit themeApplied();
        QObject::disconnect(*errConn);
        QObject::disconnect(*okConn);
        delete errConn;
        delete okConn;
        delete failed;
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
    auto* failed = new bool(false);
    QMetaObject::Connection* errConn = new QMetaObject::Connection;
    QMetaObject::Connection* okConn = new QMetaObject::Connection;
    *errConn = QObject::connect(&_fonts, &FontApplier::error, this,
                                [this, failed, errConn, okConn](const QString& message) {
        *failed = true;
        emit themeRestoreFailed(message);
        QObject::disconnect(*errConn);
        QObject::disconnect(*okConn);
        delete errConn;
        delete okConn;
        delete failed;
    });
    *okConn = QObject::connect(&_fonts, &FontApplier::restored, this,
                               [this, failed, errConn, okConn]() {
        if(!*failed)
            emit themeRestored();
        QObject::disconnect(*errConn);
        QObject::disconnect(*okConn);
        delete errConn;
        delete okConn;
        delete failed;
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
    return this->hasNative(index) || this->hasApk(index) || this->hasJolla(index)
           || this->hasIconOverlay(index);
}

bool ThemePackModel::hasNative(int index) const
{
    return this->hasCapability(index, "native");
}

bool ThemePackModel::hasJolla(int index) const
{
    return this->hasCapability(index, "jolla");
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

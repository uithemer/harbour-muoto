#include "themepackmodel.h"
#include "spawner.h"
#include <unistd.h>
#include <algorithm>
#include <QDebug>

#define setuid_ex(x) if(setuid(x)) { }
#define RAW_PACK_NAME(x) QString(x).replace("harbour-themepack-", "")

ThemePackModel::ThemePackModel(QObject *parent) : QAbstractListModel(parent)
{
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
    Spawner::execute("/usr/share/sailfishos-uithemer/themeapply.sh", SPAWN_ARGS(RAW_PACK_NAME(this->_packlist[index]) << QString::number(font) << weight), [this]() { emit themeApplied(); });
}

void ThemePackModel::restoreTheme(bool font)
{
    Spawner::execute("/usr/share/sailfishos-uithemer/themerestore.sh", SPAWN_ARGS(QString::number(font)), [this]() { emit themeRestored(); });
}

void ThemePackModel::applyADPI(const QString& adpi)
{
    Spawner::executeSync("/usr/share/sailfishos-uithemer/apply_adpi.sh " + adpi);
}

void ThemePackModel::restoreDpi(bool dpr, bool adpi)
{
    Spawner::execute("/usr/share/sailfishos-uithemer/restore_dpi.sh", SPAWN_ARGS(QString::number(dpr) << QString::number(adpi)), [this]() { emit dpiRestored(); });
}

void ThemePackModel::ocr() 
{
    Spawner::execute("/usr/share/sailfishos-uithemer/ocr.sh", [this]() { emit ocrRestored(); });
}

void ThemePackModel::recoveryTheme(bool font)
{
    Spawner::execute("/usr/share/sailfishos-uithemer/themerecovery.sh", SPAWN_ARGS(QString::number(font)), [this]() { emit themeRecovered(); });
}

void ThemePackModel::uninstall(int index)
{
    QString package = Spawner::executeSync("rpm -qf /usr/share/" + this->_packlist[index] + "/ --queryformat '%{NAME}\n'");

    Spawner::execute("rpm", SPAWN_ARGS("-e" << package), [this, index]() {
        this->beginRemoveRows(QModelIndex(), index, index);
        this->_packlist.removeAt(index);
        this->_packnames.removeAt(index);
        this->endRemoveRows();

        emit uninstallCompleted();
    });
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

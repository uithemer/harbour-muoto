#include "themepackmodel.h"
#include "spawner.h"
#include <unistd.h>
#include <algorithm>
#include <QDebug>

#define setuid_ex(x) if(setuid(x)) { }
#define RAW_PACK_NAME(x) QString(x).replace("harbour-themepack-", "")

ThemePackModel::ThemePackModel(QObject *parent) : QAbstractListModel(parent)
{
    // Errors are best-effort logged; the busy spinner in QML stays on
    // until applied/restored/recovered actually fires.
    QObject::connect(&_fonts, &FontApplier::error,
                     this, [](const QString& m) { qWarning() << "FontApplier error:" << m; });

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

void ThemePackModel::restoreDpi(bool dpr)
{
    Spawner::execute("/usr/share/sailfishos-uithemer/restore_dpi.sh", SPAWN_ARGS(QString::number(dpr)), [this]() { emit dpiRestored(); });
}

void ThemePackModel::ocr() 
{
    Spawner::execute("/usr/share/sailfishos-uithemer/ocr.sh", [this]() { emit ocrRestored(); });
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
    // onThemeRestored handler would also fire while OptionsPage is on top
    // of the page stack).
    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = QObject::connect(&_fonts, &FontApplier::restored, this,
                             [this, conn]() {
        emit themeRecovered();
        QObject::disconnect(*conn);
        delete conn;
    });
    _fonts.recoveryFonts();
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

#include "fontcarouselmodel.h"
#include "themepackmodel.h"

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace {

QString pickPreferredBasename(const QStringList& files)
{
    static const char* prefs[] = {
        "regular", "light", "thin", "book", "normal", "extralight", "medium"
    };
    for(size_t p = 0; p < sizeof(prefs) / sizeof(prefs[0]); ++p)
    {
        const QString needle = QString::fromLatin1(prefs[p]);
        for(int i = 0; i < files.size(); ++i)
        {
            const QString base = QFileInfo(files.at(i)).completeBaseName();
            if(base.toLower().contains(needle))
                return base;
        }
    }
    if(files.isEmpty())
        return QString();
    return QFileInfo(files.first()).completeBaseName();
}

} // namespace

FontCarouselModel::FontCarouselModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

ThemePackModel* FontCarouselModel::packModel() const
{
    return m_packModel;
}

void FontCarouselModel::setPackModel(ThemePackModel* model)
{
    if(m_packModel == model)
        return;

    if(m_packModel)
        disconnect(m_packModel, 0, this, 0);

    m_packModel = model;
    if(m_packModel)
        connect(m_packModel, &QAbstractItemModel::modelReset,
                this, &FontCarouselModel::rebuild);

    emit packModelChanged();
    rebuild();
}

QHash<int, QByteArray> FontCarouselModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PackIndexRole] = "packIndex";
    roles[PackNameRole] = "packName";
    roles[PackDisplayNameRole] = "packDisplayName";
    roles[SampleFontBasenameRole] = "sampleFontBasename";
    roles[IsDefaultRole] = "isDefault";
    return roles;
}

QVariant FontCarouselModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();

    const Row& row = m_rows.at(index.row());
    switch(role)
    {
    case PackIndexRole:
        return row.packIndex;
    case PackNameRole:
        return row.packName;
    case PackDisplayNameRole:
        return row.packDisplayName;
    case SampleFontBasenameRole:
        return row.sampleFontBasename;
    case IsDefaultRole:
        return row.isDefault;
    default:
        return QVariant();
    }
}

int FontCarouselModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return m_rows.size();
}

int FontCarouselModel::rowForPackIndex(int packIndex) const
{
    for(int i = 0; i < m_rows.size(); ++i)
    {
        if(m_rows.at(i).packIndex == packIndex)
            return i;
    }
    return -1;
}

QString FontCarouselModel::sampleBasenameForPack(const QString& packName) const
{
    if(packName.isEmpty())
        return QString();
    QDir dir(QStringLiteral("/usr/share/%1/font").arg(packName));
    if(!dir.exists())
        return QString();
    const QStringList files = dir.entryList(
                QStringList() << QStringLiteral("*.ttf") << QStringLiteral("*.ttc"),
                QDir::Files, QDir::Name);
    return pickPreferredBasename(files);
}

void FontCarouselModel::rebuild()
{
    beginResetModel();
    m_rows.clear();

    Row def;
    def.packIndex = -1;
    def.packName = QString();
    def.packDisplayName = QCoreApplication::translate("FontCarouselModel", "Default");
    def.sampleFontBasename = QStringLiteral("Light");
    def.isDefault = true;
    m_rows.append(def);

    if(m_packModel)
    {
        const int n = m_packModel->rowCount(QModelIndex());
        for(int i = 0; i < n; ++i)
        {
            if(!m_packModel->hasFont(i) && !m_packModel->hasFontNonLatin(i))
                continue;
            Row row;
            row.packIndex = i;
            row.packName = m_packModel->packName(i);
            row.packDisplayName = m_packModel->packDisplayName(i);
            row.sampleFontBasename = sampleBasenameForPack(row.packName);
            row.isDefault = false;
            m_rows.append(row);
        }
    }

    endResetModel();
}

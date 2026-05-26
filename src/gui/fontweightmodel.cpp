#include "fontweightmodel.h"
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>

FontWeightModel::FontWeightModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(this, &FontWeightModel::firstWeightChanged, this, &FontWeightModel::firstWeightFontChanged);
}

QString FontWeightModel::firstWeightFont() const
{
    return this->_firstweight;
}

QString FontWeightModel::firstWeight() const
{
    return QFileInfo(this->_firstweight).fileName();
}

QString FontWeightModel::packName() const
{
    return this->_packname;
}

void FontWeightModel::setPackName(const QString &packname)
{
    if(this->_packname == packname)
        return;

    this->beginResetModel();
    this->_packname = packname;
    this->loadFontWeigths();
    this->endResetModel();

    emit packNameChanged();
}

QHash<int, QByteArray> FontWeightModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[FontWeightModel::FontDisplayWeightRole] = "fontDisplayWeight";
    roles[FontWeightModel::FontWeightRole] = "fontWeight";
    return roles;
}

QVariant FontWeightModel::data(const QModelIndex &index, int role) const
{
    if(role == FontWeightModel::FontWeightRole)
        return QString(this->_weights[index.row()]).replace(".ttf", "");

    if(role == FontWeightModel::FontDisplayWeightRole)
        return this->displayWeightName(this->_weights[index.row()]);

    return QVariant();
}

int FontWeightModel::rowCount(const QModelIndex &) const
{
    return this->_weights.count();
}

QString FontWeightModel::displayWeightName(const QString& w) const
{
    QRegularExpression regex("[\\-]*([^\\.]+)\\.[a-z]+");
    QRegularExpressionMatch m = regex.match(w);

    if(!m.hasMatch())
        return w;

    return m.captured(1);
}

void FontWeightModel::loadFontWeigths()
{
    if(this->_packname.isEmpty())
        return;

    this->_weights.clear();

    QDir dir("/usr/share/" + this->_packname + "/font");

    if(!dir.exists())
        return;

    this->_weights = dir.entryList((QStringList() << "*.ttf" << "*.ttc"));
    this->_firstweight = this->_weights.first();
    emit firstWeightChanged();
}

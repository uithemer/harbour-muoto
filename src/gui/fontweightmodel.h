#ifndef FONTWEIGHTMODEL_H
#define FONTWEIGHTMODEL_H

#include <QAbstractListModel>

class FontWeightModel : public QAbstractListModel
{
    Q_PROPERTY(QString firstWeight READ firstWeight NOTIFY firstWeightChanged)
    Q_PROPERTY(QString firstWeightFont READ firstWeightFont NOTIFY firstWeightFontChanged)
    Q_PROPERTY(QString packName READ packName WRITE setPackName NOTIFY packNameChanged)

    Q_OBJECT

    public:
        enum FontWeightRoles { FontDisplayWeightRole = Qt::UserRole, FontWeightRole };

    public:
        explicit FontWeightModel(QObject *parent = 0);
        QString firstWeightFont() const;
        QString firstWeight() const;
        QString packName() const;
        void setPackName(const QString& packname);

    public:
        virtual QHash<int, QByteArray> roleNames() const;
        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual int rowCount(const QModelIndex &) const;

    private:
        QString displayWeightName(const QString &w) const;
        void loadFontWeigths();

    signals:
        void firstWeightFontChanged();
        void firstWeightChanged();
        void packNameChanged();

    private:
        QString _packname;
        QString _firstweight;
        QStringList _weights;
};

#endif // FONTWEIGHTMODEL_H

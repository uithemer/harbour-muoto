#ifndef FONTCAROUSELMODEL_H
#define FONTCAROUSELMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QVector>

class ThemePackModel;

class FontCarouselModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ThemePackModel* packModel READ packModel WRITE setPackModel
               NOTIFY packModelChanged)

public:
    enum Roles {
        PackIndexRole = Qt::UserRole,
        PackNameRole,
        PackDisplayNameRole,
        SampleFontBasenameRole,
        IsDefaultRole
    };

    explicit FontCarouselModel(QObject* parent = 0);

    ThemePackModel* packModel() const;
    void setPackModel(ThemePackModel* model);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    Q_INVOKABLE int rowForPackIndex(int packIndex) const;

signals:
    void packModelChanged();

private slots:
    void rebuild();

private:
    QString sampleBasenameForPack(const QString& packName) const;

    struct Row {
        int packIndex;
        QString packName;
        QString packDisplayName;
        QString sampleFontBasename;
        bool isDefault;
    };

    QPointer<ThemePackModel> m_packModel;
    QVector<Row> m_rows;
};

#endif // FONTCAROUSELMODEL_H

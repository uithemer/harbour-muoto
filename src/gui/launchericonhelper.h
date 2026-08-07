#ifndef LAUNCHERICONHELPER_H
#define LAUNCHERICONHELPER_H

#include <QObject>
#include <QString>

class IconPack;

class LauncherIconHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString iconPackName READ iconPackName WRITE setIconPackName NOTIFY iconPackNameChanged)

public:
    explicit LauncherIconHelper(QObject* parent = nullptr);

    QString iconPackName() const;
    void setIconPackName(const QString& name);

    Q_INVOKABLE QString iconSource(const QString& desktopPath) const;
    Q_INVOKABLE QStringList packIconIds() const;

signals:
    void iconPackNameChanged();

private:
    IconPack* m_iconPack;
    QString m_iconPackName;
};

#endif // LAUNCHERICONHELPER_H

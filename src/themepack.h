#ifndef THEMEPACK_H
#define THEMEPACK_H

#include <QObject>

class ThemePack : public QObject
{
    Q_PROPERTY(bool hasAndroidSupport READ hasAndroidSupport CONSTANT FINAL)

    Q_OBJECT

    public:
        explicit ThemePack(QObject* parent = 0);

    public slots:
        bool hasAndroidSupport() const;
        bool hasStoremanInstalled() const;
        QString whoami() const;                         // function to test what user runs app
        QString getTimer() const;                       // gets hours from timer
        qint64 getFileSize(const QString& file);
        void installDependencies();
        void restartHomescreen();
        void applyHours(const QString& hours);
        void restoreIZ();
        void enableserviceautoupdate();
        void disableserviceautoupdate();
        void enableservicesu();
        void disableservicesu();
        void hideIcon();                          // hides icon of original app, so user does not have to have two same icons on home screen

    signals:
        void dependenciesInstalled();
        void homescreenRestarted();
        void serviceChanged();
};

#endif // THEMEPACK_H


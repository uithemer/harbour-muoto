#ifndef PKTXWATCH_H
#define PKTXWATCH_H

#include <QObject>
#include <QString>

class InstallListener;

class PkTxWatch : public QObject
{
    Q_OBJECT
public:
    PkTxWatch(const QString& path, InstallListener* listener, QObject* parent = nullptr);

    QString path() const { return _path; }
    void markRelevant();

public slots:
    void onPackage(uint info, const QStringList& packageIds, const QString& summary);
    void onFinished(uint exitCode, uint runtime);

private:
    QString _path;
    InstallListener* _listener;
    bool _relevant = false;
};

#endif

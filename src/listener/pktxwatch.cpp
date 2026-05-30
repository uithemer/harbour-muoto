#include "pktxwatch.h"
#include "installlistener.h"

namespace
{
    constexpr uint PK_INFO_INSTALLING = 12;
    constexpr uint PK_INFO_UPDATING = 13;
}

PkTxWatch::PkTxWatch(const QString& path, InstallListener* listener, QObject* parent)
    : QObject(parent)
    , _path(path)
    , _listener(listener)
{
}

void PkTxWatch::onPackage(uint info, const QStringList& packageIds, const QString& summary)
{
    Q_UNUSED(packageIds);
    Q_UNUSED(summary);
    if(info == PK_INFO_INSTALLING || info == PK_INFO_UPDATING)
        _relevant = true;
}

void PkTxWatch::onFinished(uint exitCode, uint runtime)
{
    Q_UNUSED(runtime);
    if(_listener)
        _listener->onPkFinished(_path, exitCode, _relevant);
    deleteLater();
}

#include "pktxwatch.h"
#include "installlistener.h"

namespace
{
    // PkInfoEnum: 11 updating, 12 installing (13 is removing).
    constexpr uint PK_INFO_UPDATING = 11;
    constexpr uint PK_INFO_INSTALLING = 12;
}

PkTxWatch::PkTxWatch(const QString& path, InstallListener* listener, QObject* parent)
    : QObject(parent)
    , _path(path)
    , _listener(listener)
{
}

void PkTxWatch::markRelevant()
{
    _relevant = true;
}

void PkTxWatch::onPackage(uint info, const QString& packageId, const QString& summary)
{
    Q_UNUSED(packageId);
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

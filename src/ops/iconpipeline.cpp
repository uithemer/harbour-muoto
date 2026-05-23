#include "iconpipeline.h"
#include "iconoverlay.h"
#include "iconpackrunner.h"
#include "iconpaths.h"
#include "iconstockbackup.h"
#include "lipstickrefresh.h"

#include <QDir>
#include <QDebug>

bool IconPipeline::apply(const QString& packName, bool runPack, bool overlay) const
{
    if(packName.isEmpty() || packName == QLatin1String("default"))
        return false;

    if(!runPack && !overlay)
        return false;

    if(runPack && !QDir(IconPaths::packDir(packName)).exists())
    {
        qWarning() << "uithemer: icon pack not found" << packName;
        return false;
    }

    if(overlay && !QDir(IconPaths::packDir(packName)).exists())
    {
        qWarning() << "uithemer: icon pack not found" << packName;
        return false;
    }

    QDir().mkpath(IconPaths::tmpDir());

    IconStockBackup stock;
    if(!stock.restore())
        qWarning() << "uithemer: icon stock restore failed (continuing)";
    if(!stock.backup())
        qWarning() << "uithemer: icon stock backup failed";

    if(runPack)
    {
        IconPackRunner runner;
        if(!runner.run(packName))
            qWarning() << "uithemer: icon pack run produced no copies for" << packName;
    }

    if(overlay)
    {
        IconOverlay ov;
        if(!ov.apply(packName))
            qDebug() << "uithemer: no overlay applied for" << packName;
    }

    notifyLauncherAfterIconOp();
    return true;
}

bool IconPipeline::restore() const
{
    IconStockBackup stock;
    stock.restore();
    notifyLauncherAfterIconOp();
    return true;
}

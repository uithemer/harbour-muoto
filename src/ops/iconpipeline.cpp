#include "iconpipeline.h"
#include "iconoverlay.h"
#include "iconpackrunner.h"
#include "iconpaths.h"
#include "iconstockbackup.h"
#include "lipstickrefresh.h"

#include <QDir>
#include <QDebug>

namespace
{
    IconApplyResult fail(const QString& message)
    {
        IconApplyResult r;
        r.ok = false;
        r.message = message;
        return r;
    }
}

IconApplyResult IconPipeline::apply(const QString& packName, bool runPack, bool overlay) const
{
    if(packName.isEmpty() || packName == QLatin1String("default"))
        return fail(QStringLiteral("invalid pack"));

    if(!runPack && !overlay)
        return fail(QStringLiteral("no icon operation"));

    const QString root = IconPaths::packDir(packName);
    if(!QDir(root).exists())
    {
        qWarning() << "uithemer: icon pack not found" << packName << "at" << root;
        return fail(QStringLiteral("pack not found"));
    }

    QDir().mkpath(IconPaths::tmpDir());

    IconStockBackup stock;
    if(!stock.restore())
        qWarning() << "uithemer: icon stock restore failed (continuing)";

    if(!stock.backup())
        qWarning() << "uithemer: icon stock backup failed";

    bool anyWork = false;
    bool apkIconsTouched = false;

    if(runPack)
    {
        IconPackRunner runner;
        if(runner.runSfos(packName))
            anyWork = true;
    }

    if(overlay)
    {
        IconOverlay ov;
        if(!ov.applySfos(packName))
        {
            qWarning() << "uithemer: overlay not applied for" << packName << "root" << root;
            return fail(QStringLiteral("overlay not applied"));
        }
        anyWork = true;
    }

    if(runPack || overlay)
    {
        if(applyApkPhase(packName, runPack, overlay, &apkIconsTouched))
            anyWork = true;
    }

    if(!anyWork)
    {
        qWarning() << "uithemer: icon apply produced no work for" << packName << "runPack"
                   << runPack << "overlay" << overlay;
        return fail(QStringLiteral("pack run produced no copies"));
    }

    notifyLauncherAfterIconOp(apkIconsTouched);

    IconApplyResult r;
    r.ok = true;
    qInfo() << "uithemer: icons applied" << packName << "runPack" << runPack << "overlay"
            << overlay;
    return r;
}

bool IconPipeline::restore() const
{
    IconStockBackup stock;
    stock.restore();
    removeApkCustomDir();
    notifyLauncherAfterIconOp(false);
    return true;
}

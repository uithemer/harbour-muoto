#ifndef ICONPIPELINE_H
#define ICONPIPELINE_H

#include <QString>

struct IconApplyResult
{
    bool ok = false;
    QString message;
};

class IconPipeline
{
public:
    IconApplyResult apply(const QString& packName, bool runPack, bool overlay) const;
    bool restore() const;
};

#endif // ICONPIPELINE_H

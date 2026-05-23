#ifndef ICONPIPELINE_H
#define ICONPIPELINE_H

#include <QString>

class IconPipeline
{
public:
    bool apply(const QString& packName, bool overlay) const;
    bool restore() const;
};

#endif // ICONPIPELINE_H

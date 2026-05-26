#ifndef ICONPACKRUNNER_H
#define ICONPACKRUNNER_H

#include <QString>

class IconPackRunner
{
public:
    bool runSfos(const QString& packName) const;
    bool runApk(const QString& packName, bool* apkIconsTouched = nullptr) const;
};

#endif // ICONPACKRUNNER_H

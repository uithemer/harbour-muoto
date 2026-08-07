#ifndef SVGICONRENDER_H
#define SVGICONRENDER_H

#include "muotolauncherglobal.h"
#include <QByteArray>
#include <QImage>
#include <QSize>

MUOTO_LAUNCHER_EXPORT
QImage renderSvgIcon(const QByteArray& data, const QSize& requestedSize);

#endif // SVGICONRENDER_H

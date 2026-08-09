#include "spawner.h"

#include <cstdio>
#include <string>

namespace Spawner
{

QString executeSync(const QString &cmd)
{
    FILE* pipe = popen(cmd.toUtf8().constData(), "r");
    if (!pipe) return QStringLiteral("ERROR");
    char buffer[128];
    std::string result;

    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }

    pclose(pipe);
    return QString::fromStdString(result).simplified();
}

} // namespace Spawner

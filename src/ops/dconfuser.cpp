#include "dconfuser.h"

#include <QProcess>
#include <QProcessEnvironment>

#include <pwd.h>

bool runDconfAsDefaultUser(const QStringList& args, QString* stdOut)
{
    if(args.isEmpty())
        return false;

    struct passwd* pw = getpwnam("defaultuser");
    if(!pw)
        return false;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("XDG_RUNTIME_DIR"),
               QStringLiteral("/run/user/") + QString::number(
                   static_cast<qulonglong>(pw->pw_uid)));
    env.insert(QStringLiteral("HOME"), QString::fromUtf8(pw->pw_dir));
    env.insert(QStringLiteral("USER"), QStringLiteral("defaultuser"));
    env.insert(QStringLiteral("LOGNAME"), QStringLiteral("defaultuser"));

    QProcess p;
    p.setProcessEnvironment(env);
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(QStringLiteral("dconf"), args);
    if(!p.waitForFinished(15000))
    {
        p.kill();
        return false;
    }
    if(p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
        return false;

    if(stdOut)
        *stdOut = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

    return true;
}

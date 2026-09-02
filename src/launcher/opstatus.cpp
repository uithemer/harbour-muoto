#include "opstatus.h"
#include "launcherpaths.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

quint64 s_sequence = 0;

const char* outcomeName(OpStatus::Outcome outcome)
{
    switch(outcome)
    {
    case OpStatus::Ok:
        return "ok";
    case OpStatus::Partial:
        return "partial";
    case OpStatus::HardFailure:
        break;
    }
    return "failed";
}

} // namespace

namespace OpStatus {

QString path()
{
    return LauncherPaths::muotoShare() + QStringLiteral("/last-op.json");
}

quint64 seedSequence()
{
    QFile file(path());
    if(!file.open(QIODevice::ReadOnly))
        return 0;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if(!doc.isObject())
        return 0;

    s_sequence = static_cast<quint64>(doc.object().value(QStringLiteral("sequence")).toDouble());
    return s_sequence;
}

void record(const QString& op, Outcome outcome, const QString& message, int built, int written)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("sequence"), static_cast<double>(++s_sequence));
    obj.insert(QStringLiteral("op"), op);
    obj.insert(QStringLiteral("outcome"), QString::fromLatin1(outcomeName(outcome)));
    obj.insert(QStringLiteral("message"), message);
    obj.insert(QStringLiteral("built"), built);
    obj.insert(QStringLiteral("written"), written);
    obj.insert(QStringLiteral("when"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    const QByteArray content = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QDir().mkpath(LauncherPaths::muotoShare());
    const QString target = path();
    const QString tmpPath = target + QStringLiteral(".muoto-write");
    QFile::remove(tmpPath);

    {
        QFile tmp(tmpPath);
        if(!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qWarning() << "muoto-launcher: could not stage op status";
            return;
        }
        if(tmp.write(content) != content.size())
        {
            tmp.close();
            QFile::remove(tmpPath);
            qWarning() << "muoto-launcher: short write staging op status";
            return;
        }
        tmp.flush();
    }

    QFile::remove(target);
    if(!QFile::rename(tmpPath, target))
    {
        QFile::remove(tmpPath);
        qWarning() << "muoto-launcher: could not place op status";
    }
}

} // namespace OpStatus

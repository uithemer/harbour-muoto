#ifndef HARBOURTHEMEPACK_H
#define HARBOURTHEMEPACK_H

#include "iconpack.h"
#include "muotolauncherglobal.h"
#include <QDate>
#include <QHash>
#include <QStringList>

class MUOTO_LAUNCHER_EXPORT HarbourThemePack : public IconPack
{
public:
    explicit HarbourThemePack(const QString& path, QObject* parent = nullptr);

    QString displayName() override;
    QStringList icons() override;
    QStringList previewIcons() override;
    QString iconByPackageName(const QString& packageName) override;
    QString iconByActivity(const QString& activity) override;
    QImage requestIcon(const QString& iconId, const QSize& requestedSize) override;
    QImage requestClockDialIcon(const QSize& requestedSize) override;
    QImage requestHoursHandIcon(const QSize& requestedSize) override;
    QImage requestMinutesHandIcon(const QSize& requestedSize) override;
    QImage requestCalendarIcon(const QDate& date, const QSize& requestedSize) override;

    static IconPack* byShortName(const QString& shortName);

private:
    QString findJollaIcon(const QString& iconName);
    QString findNativeIcon(const QString& iconName);
    QString findApkIcon(const QString& iconName);
    void loadPreviewIcons();

    QString m_path;
    QString m_displayName;
    QStringList m_icons;
    QStringList m_previewIcons;

    QHash<QString, QString> m_jollaIcons;
    QHash<QString, QString> m_nativeIcons;
    QHash<QString, QString> m_apkIcons;

    QString m_clockDial;
    QString m_hoursHand;
    QString m_minutesHand;
    QString m_calendarIcons[31];
};

#endif // HARBOURTHEMEPACK_H

#include "mcelpmsettings.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QQmlEngine>
#include <QJSEngine>
#include <QDebug>

namespace
{
    const char* kService = "com.nokia.mce";
    const char* kPath = "/com/nokia/mce/request";
    const char* kIface = "com.nokia.mce.request";

    const char* kUseLpm = "/system/osso/dsm/display/use_low_power_mode";
    const char* kLpmTriggering = "/system/osso/dsm/locks/lpm_triggering";
    const char* kPsOnDemand = "/system/osso/dsm/proximity/on_demand";

    // Bitmask: from-pocket=1, hover-over=2
    const int kTriggerFromPocket = 1;
    const int kTriggerFromPocketAndHover = 1 | 2;
}

MceLpmSettings::MceLpmSettings()
    : QObject(nullptr)
    , _enabled(false)
    , _available(false)
{
    refresh();
}

MceLpmSettings::~MceLpmSettings()
{
}

MceLpmSettings* MceLpmSettings::instance()
{
    static MceLpmSettings* s = new MceLpmSettings();
    return s;
}

QObject* MceLpmSettings::qmlSingleton(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    MceLpmSettings* s = instance();
    QQmlEngine::setObjectOwnership(s, QQmlEngine::CppOwnership);
    return s;
}

bool MceLpmSettings::getBool(const QString& key, bool* ok)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService),
        QString::fromLatin1(kPath),
        QString::fromLatin1(kIface),
        QStringLiteral("get_config"));
    msg << key;

    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if(reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
    {
        if(ok)
            *ok = false;
        return false;
    }

    const QVariant v = reply.arguments().at(0);
    QVariant inner = v;
    if(v.canConvert<QDBusVariant>())
        inner = qvariant_cast<QDBusVariant>(v).variant();

    if(ok)
        *ok = true;
    return inner.toBool();
}

bool MceLpmSettings::setBool(const QString& key, bool value)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService),
        QString::fromLatin1(kPath),
        QString::fromLatin1(kIface),
        QStringLiteral("set_config"));
    msg << key << QVariant::fromValue(QDBusVariant(QVariant(value)));

    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if(reply.type() != QDBusMessage::ReplyMessage)
    {
        qWarning() << "MceLpmSettings: set_config failed for" << key
                    << reply.errorMessage();
        return false;
    }
    if(!reply.arguments().isEmpty() && !reply.arguments().at(0).toBool())
        return false;
    return true;
}

bool MceLpmSettings::setInt(const QString& key, int value)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService),
        QString::fromLatin1(kPath),
        QString::fromLatin1(kIface),
        QStringLiteral("set_config"));
    msg << key << QVariant::fromValue(QDBusVariant(QVariant(value)));

    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if(reply.type() != QDBusMessage::ReplyMessage)
    {
        qWarning() << "MceLpmSettings: set_config failed for" << key
                    << reply.errorMessage();
        return false;
    }
    if(!reply.arguments().isEmpty() && !reply.arguments().at(0).toBool())
        return false;
    return true;
}

void MceLpmSettings::refresh()
{
    bool ok = false;
    const bool lpm = getBool(QString::fromLatin1(kUseLpm), &ok);
    if(!ok)
    {
        if(_available)
        {
            _available = false;
            emit availableChanged();
        }
        return;
    }

    if(!_available)
    {
        _available = true;
        emit availableChanged();
    }

    if(_enabled != lpm)
    {
        _enabled = lpm;
        emit enabledChanged();
    }
}

bool MceLpmSettings::applyProfile(bool on)
{
    bool ok = true;
    if(on)
    {
        ok = setBool(QString::fromLatin1(kUseLpm), true) && ok;
        ok = setInt(QString::fromLatin1(kLpmTriggering),
                    kTriggerFromPocketAndHover) && ok;
        ok = setBool(QString::fromLatin1(kPsOnDemand), false) && ok;
    }
    else
    {
        ok = setBool(QString::fromLatin1(kUseLpm), false) && ok;
        ok = setInt(QString::fromLatin1(kLpmTriggering),
                    kTriggerFromPocket) && ok;
        ok = setBool(QString::fromLatin1(kPsOnDemand), true) && ok;
    }

    refresh();

    if(!ok)
    {
        emit error(QStringLiteral("Could not update low-power mode settings"));
        return false;
    }
    return true;
}

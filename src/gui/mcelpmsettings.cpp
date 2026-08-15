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
    const int kTriggerHoverOver = 2;
}

MceLpmSettings::MceLpmSettings()
    : QObject(nullptr)
    , _enabled(false)
    , _triggerFromPocket(true)
    , _triggerHoverOver(false)
    , _proximityReady(false)
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

int MceLpmSettings::triggeringMask(bool fromPocket, bool hoverOver)
{
    int mask = 0;
    if(fromPocket)
        mask |= kTriggerFromPocket;
    if(hoverOver)
        mask |= kTriggerHoverOver;
    return mask;
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

int MceLpmSettings::getInt(const QString& key, bool* ok)
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
        return 0;
    }

    const QVariant v = reply.arguments().at(0);
    QVariant inner = v;
    if(v.canConvert<QDBusVariant>())
        inner = qvariant_cast<QDBusVariant>(v).variant();

    if(ok)
        *ok = true;
    return inner.toInt();
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
    bool okLpm = false;
    bool okTrig = false;
    bool okPs = false;
    const bool lpm = getBool(QString::fromLatin1(kUseLpm), &okLpm);
    const int trig = getInt(QString::fromLatin1(kLpmTriggering), &okTrig);
    const bool onDemand = getBool(QString::fromLatin1(kPsOnDemand), &okPs);

    if(!okLpm || !okTrig || !okPs)
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

    const bool fromPocket = (trig & kTriggerFromPocket) != 0;
    const bool hoverOver = (trig & kTriggerHoverOver) != 0;
    if(_triggerFromPocket != fromPocket)
    {
        _triggerFromPocket = fromPocket;
        emit triggerFromPocketChanged();
    }
    if(_triggerHoverOver != hoverOver)
    {
        _triggerHoverOver = hoverOver;
        emit triggerHoverOverChanged();
    }

    // proximityReady <=> !ps-on-demand
    const bool ready = !onDemand;
    if(_proximityReady != ready)
    {
        _proximityReady = ready;
        emit proximityReadyChanged();
    }
}

bool MceLpmSettings::apply(bool enabled, bool fromPocket, bool hoverOver,
                           bool proximityReady)
{
    bool ok = true;
    ok = setBool(QString::fromLatin1(kUseLpm), enabled) && ok;
    ok = setInt(QString::fromLatin1(kLpmTriggering),
                triggeringMask(fromPocket, hoverOver)) && ok;
    // proximityReady true => ps-on-demand disabled
    ok = setBool(QString::fromLatin1(kPsOnDemand), !proximityReady) && ok;

    refresh();

    if(!ok)
    {
        emit error(QStringLiteral("Could not update low-power mode settings"));
        return false;
    }
    return true;
}

bool MceLpmSettings::applyDefaults()
{
    // Stock: LPM off, from-pocket only, ps-on-demand on (proximityReady false).
    return apply(false, true, false, false);
}

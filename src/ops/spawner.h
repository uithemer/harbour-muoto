#ifndef SPAWNER_H
#define SPAWNER_H

#include <QObject>
#include <QProcess>
#include <QHash>
#include <functional>

// 2.6.0: setuid_ex was a no-return-value wrapper for callers that did
// `setuid(0)` to elevate the GUI process to root before spawning a
// privileged shell. With the GUI now running as defaultuser and every
// privileged op going through harbour-muoto-helperd over D-Bus,
// no in-process call site needs root anymore. The macro is gone.
#define SPAWN_ARGS(...) (QStringList() << __VA_ARGS__)

class Spawner : public QObject
{
    Q_OBJECT

    private:
        explicit Spawner(QObject *parent = nullptr);

    public:
        static QString executeSync(const QString& cmd);
        static void execute(const QString& command, const QStringList& arguments, std::function<void()> done);
        static void execute(const QString& command, std::function<void()> done);

    private:
        static QHash< QProcess*, std::function<void()> > _callbackmap;
};

#endif // SPAWNER_H

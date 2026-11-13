#include "ZSSessionRepository.h"
#include "ZSPlatformPaths.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

ZSSessionRepository::ZSSessionRepository(QObject *parent)
    : QObject(parent)
{
}

QStringList ZSSessionRepository::profileNames() const
{
    const QJsonObject root = readStore();
    const QJsonObject profiles = root.value(QStringLiteral("profiles")).toObject();
    return profiles.keys();
}

bool ZSSessionRepository::saveProfile(const QString &name, const ZSSerialProfile &p)
{
    if (name.isEmpty())
        return false;
    QJsonObject root = readStore();
    QJsonObject profiles = root.value(QStringLiteral("profiles")).toObject();
    profiles.insert(name, objectFromProfile(p));
    root.insert(QStringLiteral("profiles"), profiles);
    writeStore(root);
    return true;
}

bool ZSSessionRepository::deleteProfile(const QString &name)
{
    QJsonObject root = readStore();
    QJsonObject profiles = root.value(QStringLiteral("profiles")).toObject();
    if (!profiles.contains(name))
        return false;
    profiles.remove(name);
    root.insert(QStringLiteral("profiles"), profiles);
    writeStore(root);
    return true;
}

ZSSerialProfile ZSSessionRepository::loadProfile(const QString &name) const
{
    const QJsonObject root = readStore();
    const QJsonObject profiles = root.value(QStringLiteral("profiles")).toObject();
    const QJsonValue v = profiles.value(name);
    if (v.isObject())
        return profileFromObject(v.toObject());
    return ZSSerialProfile{};
}

ZSSerialProfile ZSSessionRepository::lastProfile() const
{
    const QJsonObject root = readStore();
    const QJsonValue v = root.value(QStringLiteral("last"));
    if (v.isObject())
        return profileFromObject(v.toObject());
    return ZSSerialProfile{};
}

void ZSSessionRepository::setLastProfile(const ZSSerialProfile &p)
{
    QJsonObject root = readStore();
    root.insert(QStringLiteral("last"), objectFromProfile(p));
    writeStore(root);
}

ZSSerialProfile ZSSessionRepository::profileFromObject(const QJsonObject &obj) const
{
    ZSSerialProfile p;
    p.port = obj.value(QStringLiteral("port")).toString();
    p.baudRate = obj.value(QStringLiteral("baudRate")).toInt(9600);
    p.dataBits = obj.value(QStringLiteral("dataBits")).toInt(8);
    p.stopBits = obj.value(QStringLiteral("stopBits")).toInt(1);
    p.parity = obj.value(QStringLiteral("parity")).toInt(0);
    p.flow = obj.value(QStringLiteral("flow")).toInt(0);
    p.codepage = obj.value(QStringLiteral("codepage")).toString();
    return p;
}

QJsonObject ZSSessionRepository::objectFromProfile(const ZSSerialProfile &p) const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("port"), p.port);
    obj.insert(QStringLiteral("baudRate"), p.baudRate);
    obj.insert(QStringLiteral("dataBits"), p.dataBits);
    obj.insert(QStringLiteral("stopBits"), p.stopBits);
    obj.insert(QStringLiteral("parity"), p.parity);
    obj.insert(QStringLiteral("flow"), p.flow);
    obj.insert(QStringLiteral("codepage"), p.codepage);
    return obj;
}

void ZSSessionRepository::writeStore(const QJsonObject &root) const
{
    const QString path = ZSPlatformPaths::sessionStorePath();
    const QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[ZSSessionRepository] cannot open for write:" << path;
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QJsonObject ZSSessionRepository::readStore() const
{
    const QString path = ZSPlatformPaths::sessionStorePath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QJsonObject{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return QJsonObject{};
    return doc.object();
}

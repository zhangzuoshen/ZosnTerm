#include "ZSConfigStore.h"
#include "ZSPlatformPaths.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

ZSConfigStore::ZSConfigStore(QObject *parent)
    : QObject(parent)
{
    loadFromDisk();
}

QVariant ZSConfigStore::value(const QString &key, const QVariant &defaultValue) const
{
    auto it = m_cache.find(key);
    if (it == m_cache.end())
        return defaultValue;
    return it.value();
}

void ZSConfigStore::setValue(const QString &key, const QVariant &value)
{
    m_cache.insert(key, value);
}

void ZSConfigStore::sync()
{
    const QString path = ZSPlatformPaths::configPath();
    QFileInfo fi(path);
    if (!QDir().mkpath(fi.absolutePath())) {
        qWarning() << "[ZSConfigStore] 无法创建配置目录" << fi.absolutePath();
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QFile::Truncate)) {
        qWarning() << "[ZSConfigStore] 无法写入配置" << path;
        return;
    }
    QJsonDocument doc(QJsonObject::fromVariantMap(m_cache));
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    qDebug() << "[ZSConfigStore] 已持久化配置到" << path << "keys =" << m_cache.size();
}

void ZSConfigStore::reload()
{
    m_cache.clear();
    loadFromDisk();
}

void ZSConfigStore::loadFromDisk()
{
    const QString path = ZSPlatformPaths::configPath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        // 首次运行：无文件，使用调用方的默认值。
        return;
    }
    const QByteArray data = f.readAll();
    f.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (doc.isNull()) {
        qWarning() << "[ZSConfigStore] 配置解析失败" << err.errorString();
        return;
    }
    if (doc.isObject())
        m_cache = doc.object().toVariantMap();
}

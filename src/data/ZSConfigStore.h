#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantMap>

// 全局配置存储（M5：JSON 持久化到 ZSPlatformPaths::configPath()）。
// 采用扁平 key（如 "appearance/fontFamily"）的 QVariantMap 缓存，
// 对外提供 value/setValue/sync。QML 通过上下文属性 "ZSConfig" 直接读写。
class ZSConfigStore : public QObject
{
    Q_OBJECT

public:
    explicit ZSConfigStore(QObject *parent = nullptr);

    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void sync();
    Q_INVOKABLE void reload();

private:
    void loadFromDisk();

    QVariantMap m_cache;
};

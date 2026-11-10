#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>

// 串口连接配置（命名会话 / 上次使用配置）。
// parity 用 ZS_PAR_* 数值 (0..4)，flow 用 ZS_FLOW_* 数值 (0..3)，
// 与 ZSPuTTYBackendAdapter::setConnectionParameters 的约定一致。
struct ZSSerialProfile
{
    QString port;
    int baudRate = 9600;
    int dataBits = 8;
    int stopBits = 1;
    int parity = 0;
    int flow = 0;
};

// 会话持久化仓库（M4 落地：JSON 文件存储于 ZSPlatformPaths::sessionStorePath()）。
// 存储命名串口配置（profiles）与最近一次使用的配置（last）。
class ZSSessionRepository : public QObject
{
    Q_OBJECT

public:
    explicit ZSSessionRepository(QObject *parent = nullptr);

    // 命名配置
    Q_INVOKABLE QStringList profileNames() const;
    Q_INVOKABLE bool saveProfile(const QString &name, const ZSSerialProfile &p);
    Q_INVOKABLE bool deleteProfile(const QString &name);
    ZSSerialProfile loadProfile(const QString &name) const;

    // 最近一次使用的配置（启动时自动恢复）
    ZSSerialProfile lastProfile() const;
    void setLastProfile(const ZSSerialProfile &p);

private:
    ZSSerialProfile profileFromObject(const QJsonObject &obj) const;
    QJsonObject objectFromProfile(const ZSSerialProfile &p) const;
    void writeStore(const QJsonObject &root) const;
    QJsonObject readStore() const;
};

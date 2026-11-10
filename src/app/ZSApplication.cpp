#include "ZSApplication.h"

#include "ZSTerminalView.h"
#include "ZSPuTTYBackendAdapter.h"
#include "ZSIConnection.h"
#include "ZSSessionRepository.h"

#include <QtGlobal>   // Q_OS_WIN
#include <QSerialPortInfo>
#include <QDebug>

ZSApplication::ZSApplication(QObject *parent)
    : QObject(parent)
    , m_statusMessage(QStringLiteral("就绪"))
    , m_repo(new ZSSessionRepository(this))
{
}

QString ZSApplication::statusMessage() const
{
    return m_statusMessage;
}

void ZSApplication::setStatusMessage(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged(m_statusMessage);
    }
}

void ZSApplication::setTerminalView(ZSTerminalView *view)
{
    m_view = view;
}

QStringList ZSApplication::availablePorts() const
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const auto &info : infos)
        ports.append(info.portName());
    if (ports.isEmpty()) {
        // 无可用串口时给平台默认占位，方便手动输入/测试
#if defined(Q_OS_WIN)
        ports.append(QStringLiteral("COM1"));
#else
        ports.append(QStringLiteral("/dev/ttyS0"));
#endif
    }
    return ports;
}

void ZSApplication::openWith(const QString &port, int baudRate, int dataBits,
                            int stopBits, int parity, int flow)
{
    if (!m_view) {
        setStatusMessage(QStringLiteral("未找到终端视图 (ZSTerminalView)"));
        return;
    }
    if (m_adapter)
        closeSerialSession();

    auto *adapter = new ZSPuTTYBackendAdapter(this);
    adapter->setView(m_view);
    adapter->setConnectionParameters(port, baudRate, dataBits, stopBits, parity, flow);

    connect(adapter, &ZSPuTTYBackendAdapter::statusMessage,
            this, &ZSApplication::setStatusMessage);
    connect(adapter, &ZSIConnection::stateChanged, this,
            [this](ZSIConnection::State s) {
                const char *txt = "未知";
                switch (s) {
                case ZSIConnection::State::Connecting:   txt = "连接中"; break;
                case ZSIConnection::State::Connected:     txt = "已连接"; break;
                case ZSIConnection::State::Disconnected:  txt = "已断开"; break;
                case ZSIConnection::State::Error:         txt = "错误";   break;
                }
                setStatusMessage(QStringLiteral("串口状态：%1").arg(QString::fromLatin1(txt)));
            });

    m_adapter = adapter;

    if (!adapter->open()) {
        setStatusMessage(
            QStringLiteral("打开串口失败（检查设备 %1 与权限）").arg(port));
    }
}

void ZSApplication::connectSerial(const QString &port, int baudRate, int dataBits,
                                 int stopBits, int parity, int flow)
{
    // M4：记住本次使用的配置，供下次启动自动恢复。
    ZSSerialProfile last;
    last.port = port;
    last.baudRate = baudRate;
    last.dataBits = dataBits;
    last.stopBits = stopBits;
    last.parity = parity;
    last.flow = flow;
    m_repo->setLastProfile(last);

    openWith(port, baudRate, dataBits, stopBits, parity, flow);
}

void ZSApplication::newSerialSession()
{
    // 阶段一默认参数；真实配置由 QML 经 connectSerial() 提供。
#if defined(Q_OS_WIN)
    const QString defaultPort = QStringLiteral("COM1");
#else
    const QString defaultPort = QStringLiteral("/dev/ttyS0");
#endif
    openWith(defaultPort, 9600, 8, 1, ZS_PAR_NONE, ZS_FLOW_NONE);
}

void ZSApplication::closeSerialSession()
{
    if (m_adapter) {
        m_adapter->close();
        m_adapter->deleteLater();
        m_adapter = nullptr;
    }
}

/* ---------------- M4 会话管理 ---------------- */

QStringList ZSApplication::sessionNames() const
{
    return m_repo->profileNames();
}

void ZSApplication::saveSession(const QString &name, const QString &port,
                               int baudRate, int dataBits, int stopBits,
                               int parity, int flow)
{
    QString nm = name.trimmed();
    if (nm.isEmpty()) {
        setStatusMessage(QStringLiteral("会话名不能为空"));
        return;
    }
    ZSSerialProfile p;
    p.port = port;
    p.baudRate = baudRate;
    p.dataBits = dataBits;
    p.stopBits = stopBits;
    p.parity = parity;
    p.flow = flow;
    m_repo->saveProfile(nm, p);
    setStatusMessage(QStringLiteral("已保存会话：%1").arg(nm));
}

void ZSApplication::loadSession(const QString &name)
{
    ZSSerialProfile p = m_repo->loadProfile(name);
    if (p.port.isEmpty()) {
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
        return;
    }
    emit connectionParamsApplied(p.port, p.baudRate, p.dataBits,
                                 p.stopBits, p.parity, p.flow);
}

void ZSApplication::deleteSession(const QString &name)
{
    if (m_repo->deleteProfile(name))
        setStatusMessage(QStringLiteral("已删除会话：%1").arg(name));
    else
        setStatusMessage(QStringLiteral("未找到会话：%1").arg(name));
}

QVariantMap ZSApplication::lastParams() const
{
    ZSSerialProfile p = m_repo->lastProfile();
    QVariantMap m;
    m.insert(QStringLiteral("port"), p.port);
    m.insert(QStringLiteral("baudRate"), p.baudRate);
    m.insert(QStringLiteral("dataBits"), p.dataBits);
    m.insert(QStringLiteral("stopBits"), p.stopBits);
    m.insert(QStringLiteral("parity"), p.parity);
    m.insert(QStringLiteral("flow"), p.flow);
    return m;
}

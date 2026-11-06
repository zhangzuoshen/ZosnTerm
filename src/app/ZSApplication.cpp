#include "ZSApplication.h"

#include "ZSTerminalView.h"
#include "ZSPuTTYBackendAdapter.h"
#include "ZSIConnection.h"

#include <QDebug>

ZSApplication::ZSApplication(QObject *parent)
    : QObject(parent)
    , m_statusMessage(QStringLiteral("就绪"))
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

void ZSApplication::newSerialSession()
{
    if (!m_view) {
        setStatusMessage(QStringLiteral("未找到终端视图 (ZSTerminalView)"));
        return;
    }
    if (m_adapter) {
        closeSerialSession();
    }

    auto *adapter = new ZSPuTTYBackendAdapter(this);
    adapter->setView(m_view);

    // M2 demo 参数。真实参数后续由会话配置提供（M4）。
    // 注意：本机若无 /dev/ttyS0 设备，open() 会失败并上报错误信息，
    // 属预期行为，证明串口→引擎链路已接通。
    adapter->setConnectionParameters(
        QStringLiteral("/dev/ttyS0"), 9600, 8, 1, ZS_PAR_NONE, ZS_FLOW_NONE);

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
        setStatusMessage(QStringLiteral("打开串口失败（检查设备 /dev/ttyS0 与权限）"));
    }
}

void ZSApplication::closeSerialSession()
{
    if (m_adapter) {
        m_adapter->close();
        m_adapter->deleteLater();
        m_adapter = nullptr;
    }
}

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>

class ZSTerminalView;        /* forward decl; full def pulled in by .cpp */
class ZSPuTTYBackendAdapter; /* forward decl */

// 应用根对象：作为 QML 上下文属性 "ZSApplication" 暴露给 UI。
// M2：持有并驱动一个 ZSPuTTYBackendAdapter（内嵌 PuTTY 终端仿真引擎），
// 把串行数据接到 QML 中的 ZSTerminalView。
// M3：补全串口参数配置与连接闭环——UI 可配置端口/波特率/数据位/停止位/
// 校验/流控，经 connectSerial() 传入并真正打开 QSerialPort。
class ZSApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ZSApplication(QObject *parent = nullptr);
    QString statusMessage() const;

    /* 由 main() 在 QML 加载后调用：把 QML 树里的 ZSTerminalView 交给控制器。 */
    void setTerminalView(ZSTerminalView *view);

    /* 枚举当前系统可用串口名（如 COM3 / /dev/ttyUSB0）。供 QML 下拉填充。 */
    Q_INVOKABLE QStringList availablePorts() const;

    /* 按给定参数建立串口会话。参数含义见 ZSPuTTYBackendAdapter::
     * setConnectionParameters；parity/flow 用 ZS_PAR_* / ZS_FLOW_* 数值
     * （0..4 = None/Odd/Even/Mark/Space，0/1/2 = None/XONXOFF/RTSCTS）。 */
    Q_INVOKABLE void connectSerial(const QString &port, int baudRate,
                                   int dataBits, int stopBits,
                                   int parity, int flow);

    /* 无参快捷连接（阶段一默认参数）；保留以兼容旧入口。 */
    Q_INVOKABLE void newSerialSession();
    Q_INVOKABLE void closeSerialSession();

signals:
    void statusMessageChanged(const QString &msg);

private:
    void openWith(const QString &port, int baudRate, int dataBits,
                  int stopBits, int parity, int flow);
    void setStatusMessage(const QString &msg);

    QString m_statusMessage;
    QPointer<ZSTerminalView> m_view;
    QPointer<ZSPuTTYBackendAdapter> m_adapter;
};

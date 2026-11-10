#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QVariantMap>

class ZSTerminalView;        /* forward decl; full def pulled in by .cpp */
class ZSPuTTYBackendAdapter; /* forward decl */
class ZSSessionRepository;   /* forward decl; M4 会话持久化 */

// 应用根对象：作为 QML 上下文属性 "ZSApplication" 暴露给 UI。
// M2：持有并驱动一个 ZSPuTTYBackendAdapter（内嵌 PuTTY 终端仿真引擎），
// 把串行数据接到 QML 中的 ZSTerminalView。
// M3：补全串口参数配置与连接闭环——UI 可配置端口/波特率/数据位/停止位/
// 校验/流控，经 connectSerial() 传入并真正打开 QSerialPort。
// M4：会话管理——保存/恢复命名串口配置，启动时自动填充上次配置。
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

    /* 按给定参数建立串口会话。参数约定见 ZSPuTTYBackendAdapter::
     * setConnectionParameters；parity/flow 用 ZS_PAR_* / ZS_FLOW_* 数值
     * （parity 0..4 = None/Odd/Even/Mark/Space；flow 0..3 = None/XONXOFF/RTSCTS/DSRDTR）。 */
    Q_INVOKABLE void connectSerial(const QString &port, int baudRate,
                                   int dataBits, int stopBits,
                                   int parity, int flow);

    /* 无参快捷连接（阶段一默认参数）；保留以兼容旧入口。 */
    Q_INVOKABLE void newSerialSession();
    Q_INVOKABLE void closeSerialSession();

    /* ---- M4 会话管理 ---- */
    /* 已保存的命名会话名列表。 */
    Q_INVOKABLE QStringList sessionNames() const;
    /* 保存当前参数为命名会话（覆盖同名）。 */
    Q_INVOKABLE void saveSession(const QString &name, const QString &port,
                                 int baudRate, int dataBits, int stopBits,
                                 int parity, int flow);
    /* 加载命名会话：成功则发 connectionParamsApplied 让 UI 回填。 */
    Q_INVOKABLE void loadSession(const QString &name);
    /* 删除命名会话。 */
    Q_INVOKABLE void deleteSession(const QString &name);
    /* 上次使用的配置（QML 启动时恢复）。键：port/baudRate/dataBits/stopBits/parity/flow。 */
    Q_INVOKABLE QVariantMap lastParams() const;

signals:
    void statusMessageChanged(const QString &msg);
    /* 加载会话/恢复配置后，UI 据此把参数回填到各 ComboBox。 */
    void connectionParamsApplied(const QString &port, int baudRate, int dataBits,
                                 int stopBits, int parity, int flow);

private:
    void openWith(const QString &port, int baudRate, int dataBits,
                  int stopBits, int parity, int flow);
    void setStatusMessage(const QString &msg);

    QString m_statusMessage;
    QPointer<ZSTerminalView> m_view;
    QPointer<ZSPuTTYBackendAdapter> m_adapter;
    ZSSessionRepository *m_repo = nullptr;
};

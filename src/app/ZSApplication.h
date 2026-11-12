#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QVariantMap>

class ZSTerminalView;        /* forward decl; full def pulled in by .cpp */
class ZSPuTTYBackendAdapter; /* forward decl */
class ZSSessionRepository;   /* forward decl; M4 会话持久化 */
class ZSConfigStore;         /* forward decl; M5 全局设置 */

// 应用根对象：作为 QML 上下文属性 "ZSApplication" 暴露给 UI。
// M2：持有并驱动一个 ZSPuTTYBackendAdapter（内嵌 PuTTY 终端仿真引擎）。
// M3：补全串口参数配置与连接闭环。
// M4：会话管理——保存/恢复命名串口配置，启动时自动填充上次配置。
// M5：外观/字体/主题设置——从 ZSConfigStore 读取并套用到终端视图。
class ZSApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ZSApplication(QObject *parent = nullptr);
    QString statusMessage() const;

    /* 由 main() 在 QML 加载后调用：把 QML 树里的 ZSTerminalView 交给控制器。 */
    void setTerminalView(ZSTerminalView *view);
    /* M5：注入全局配置存储（在 setTerminalView 之前调用）。 */
    void setConfigStore(ZSConfigStore *store);

    /* 枚举当前系统可用串口名（如 COM3 / /dev/ttyUSB0）。供 QML 下拉填充。 */
    Q_INVOKABLE QStringList availablePorts() const;

    /* 按给定参数建立串口会话。parity/flow 用 ZS_PAR_* / ZS_FLOW_* 数值。 */
    Q_INVOKABLE void connectSerial(const QString &port, int baudRate,
                                   int dataBits, int stopBits,
                                   int parity, int flow);

    /* 无参快捷连接（阶段一默认参数）；保留以兼容旧入口。 */
    Q_INVOKABLE void newSerialSession();
    Q_INVOKABLE void closeSerialSession();

    /* ---- M4 会话管理 ---- */
    Q_INVOKABLE QStringList sessionNames() const;
    Q_INVOKABLE void saveSession(const QString &name, const QString &port,
                                 int baudRate, int dataBits, int stopBits,
                                 int parity, int flow);
    Q_INVOKABLE void loadSession(const QString &name);
    Q_INVOKABLE void deleteSession(const QString &name);
    Q_INVOKABLE QVariantMap lastParams() const;

    /* ---- M5 外观/字体/主题设置 ---- */
    /* 从 ZSConfigStore 读取外观设置并应用到终端视图。设置面板"应用"时调用。 */
    Q_INVOKABLE void applySettings();
    /* 等宽字体族列表，供设置面板下拉填充。 */
    Q_INVOKABLE QStringList fontFamilies() const;

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
    ZSConfigStore *m_config = nullptr;
};

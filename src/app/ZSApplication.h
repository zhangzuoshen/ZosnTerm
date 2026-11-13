#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QVariantMap>
#include <QVariantList>

class ZSTerminalView;        /* forward decl; full def pulled in by .cpp */
class ZSPuTTYBackendAdapter; /* forward decl */
class ZSSessionRepository;   /* forward decl; M4 会话持久化 */
class ZSConfigStore;         /* forward decl; M5 全局设置 */

// 应用根对象：作为 QML 上下文属性 "ZSApplication" 暴露给 UI。
// M2：持有并驱动一个 ZSPuTTYBackendAdapter（内嵌 PuTTY 终端仿真引擎）。
// M3：补全串口参数配置与连接闭环。
// M4：会话管理——保存/恢复命名串口配置，启动时自动填充上次配置。
// M5：外观/字体/主题设置——从 ZSConfigStore 读取并套用到终端视图。
// M6：MobaXTerm 风格界面——暴露会话列表 / 点选即连 / 连接状态与摘要。
class ZSApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionSummary READ connectionSummary NOTIFY connectionSummaryChanged)

public:
    explicit ZSApplication(QObject *parent = nullptr);
    QString statusMessage() const;

    /* M6：连接状态与当前连接摘要（供标签页 / 状态栏显示）。 */
    bool isConnected() const;
    QString connectionSummary() const;

    /* 由 main() 在 QML 加载后调用：把 QML 树里的 ZSTerminalView 交给控制器。 */
    void setTerminalView(ZSTerminalView *view);
    /* M5：注入全局配置存储（在 setTerminalView 之前调用）。 */
    void setConfigStore(ZSConfigStore *store);

    /* 枚举当前系统可用串口名（如 COM3 / /dev/ttyUSB0）。供 QML 下拉填充。 */
    Q_INVOKABLE QStringList availablePorts() const;

    /* 按给定参数建立串口会话。parity/flow 用 ZS_PAR_* / ZS_FLOW_* 数值。 */
    Q_INVOKABLE void connectSerial(const QString &port, int baudRate,
                                   int dataBits, int stopBits,
                                   int parity, int flow,
                                   const QString &codepage = QStringLiteral("UTF-8"));

    /* 无参快捷连接（阶段一默认参数）；保留以兼容旧入口。 */
    Q_INVOKABLE void newSerialSession();
    Q_INVOKABLE void closeSerialSession();

    /* ---- M4 会话管理 ---- */
    Q_INVOKABLE QStringList sessionNames() const;
    /* M6：返回 [{name,port,baudRate,dataBits,stopBits,parity,flow}, ...]，供侧栏 ListView 使用。 */
    Q_INVOKABLE QVariantList sessionList() const;
    Q_INVOKABLE void saveSession(const QString &name, const QString &port,
                                 int baudRate, int dataBits, int stopBits,
                                 int parity, int flow,
                                 const QString &codepage = QStringLiteral("UTF-8"));
    Q_INVOKABLE void loadSession(const QString &name);
    Q_INVOKABLE void deleteSession(const QString &name);
    /* M6：加载命名会话参数并立即连接（MobaXTerm 双击会话的行为）。 */
    Q_INVOKABLE void openSession(const QString &name);
    Q_INVOKABLE QVariantMap lastParams() const;

    /* ---- M5 外观/字体/主题设置 ---- */
    /* 从 ZSConfigStore 读取外观设置并应用到终端视图。设置面板"应用"时调用。 */
    Q_INVOKABLE void applySettings();
    /* 等宽字体族列表，供设置面板下拉填充。 */
    Q_INVOKABLE QStringList fontFamilies() const;
    /* 可选编码（串口数据解码字符集），供工具栏下拉。 */
    Q_INVOKABLE QVariantList availableEncodings() const;
    /* 依据系统区域默认编码：中文系统 GBK，其余 UTF-8。 */
    Q_INVOKABLE QString defaultEncoding() const;

signals:
    void statusMessageChanged(const QString &msg);
    /* 加载会话/恢复配置后，UI 据此把参数回填到各 ComboBox。 */
    void connectionParamsApplied(const QString &port, int baudRate, int dataBits,
                                 int stopBits, int parity, int flow,
                                 const QString &codepage);
    /* M6：连接状态变化。 */
    void connectionStateChanged(bool connected);
    /* M6：当前连接摘要（如 "COM1 · 9600 8N1"）变化。 */
    void connectionSummaryChanged(const QString &summary);

private:
    void openWith(const QString &port, int baudRate, int dataBits,
                  int stopBits, int parity, int flow,
                  const QString &codepage = QStringLiteral("UTF-8"));
    void setStatusMessage(const QString &msg);
    void setConnected(bool c);
    void setConnectionSummary(const QString &s);
    /* parity 数值 -> 字符（N/O/E/M/S）。 */
    static QChar parityChar(int parity);

    QString m_statusMessage;
    bool m_connected = false;
    QString m_connectionSummary;
    QPointer<ZSTerminalView> m_view;
    QPointer<ZSPuTTYBackendAdapter> m_adapter;
    ZSSessionRepository *m_repo = nullptr;
    ZSConfigStore *m_config = nullptr;
};

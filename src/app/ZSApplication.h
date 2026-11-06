#pragma once

#include <QObject>
#include <QString>
#include <QPointer>

class ZSTerminalView;        /* forward decl; full def pulled in by .cpp */
class ZSPuTTYBackendAdapter; /* forward decl */

// 应用根对象：作为 QML 上下文属性 "ZSApplication" 暴露给 UI。
// M2：持有并驱动一个 ZSPuTTYBackendAdapter（内嵌 PuTTY 终端仿真引擎），
// 把串行数据接到 QML 中的 ZSTerminalView。
class ZSApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ZSApplication(QObject *parent = nullptr);
    QString statusMessage() const;

    /* 由 main() 在 QML 加载后调用：把 QML 树里的 ZSTerminalView 交给控制器。 */
    void setTerminalView(ZSTerminalView *view);

    Q_INVOKABLE void newSerialSession();
    Q_INVOKABLE void closeSerialSession();

signals:
    void statusMessageChanged(const QString &msg);

private:
    void setStatusMessage(const QString &msg);

    QString m_statusMessage;
    QPointer<ZSTerminalView> m_view;
    QPointer<ZSPuTTYBackendAdapter> m_adapter;
};

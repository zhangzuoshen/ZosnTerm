#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "ZSApplication.h"
#include "ZSTerminalView.h"
#include "ZSConfigStore.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("ZosnTerm"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // M6：强制 Fusion 风格，确保自定义按钮/下拉背景（MobaXTerm 钢蓝调）生效，
    // 否则 Windows 原生风格会忽略 background 自定义导致控件回退为系统默认外观。
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    ZSApplication zsApp;
    ZSConfigStore configStore;          // M5：全局设置（构造即加载 config.json）
    zsApp.setConfigStore(&configStore);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("ZSApplication"), &zsApp);
    engine.rootContext()->setContextProperty(QStringLiteral("ZSConfig"), &configStore);
    qmlRegisterType<ZSTerminalView>("ZosnTerm", 1, 0, "ZSTerminalView");
    engine.loadFromModule("ZosnTerm", "ZSMainWindow");

    if (engine.rootObjects().isEmpty())
        return -1;

    // QML 创建了 ZSTerminalView；把它交给应用控制器，使 "新建串口" 能驱动引擎。
    ZSTerminalView *termView = nullptr;
    for (QObject *root : engine.rootObjects()) {
        termView = root ? root->findChild<ZSTerminalView *>() : nullptr;
        if (termView)
            break;
    }
    zsApp.setTerminalView(termView);
    if (!termView)
        qWarning() << "[main] 未在 QML 树中找到 ZSTerminalView";

    return app.exec();
}

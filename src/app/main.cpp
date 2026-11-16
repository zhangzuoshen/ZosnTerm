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

    // M13: 每个会话的 ZSTerminalView 由 QML 在 Component.onCompleted 中
    // 通过 ZSApplication.registerSessionView() 自行注册，无需 findChild。

    return app.exec();
}

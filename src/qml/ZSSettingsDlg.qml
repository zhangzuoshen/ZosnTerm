import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

Window {
    id: dlg
    title: "设置 · 外观"
    width: 460
    height: 540
    modality: Qt.ApplicationModal
    color: "#eef1f5"

    // 默认值（与 ZSApplication::applySettings 的 fallback 保持一致）
    readonly property string defFont: "DejaVu Sans Mono"
    readonly property int    defSize: 14
    readonly property string defFg: "#c0c0c0"
    readonly property string defBg: "#000000"
    readonly property string defCursor: "#c0c0c0"
    readonly property int    defShape: 0

    property bool loading: false

    function cfgStr(k, d) { var v = ZSConfig.value(k, d); return (v === undefined || v === null) ? d : v }
    function cfgInt(k, d) { var v = ZSConfig.value(k, d); return (v === undefined || v === null) ? d : v }

    // 把当前控件状态写回 ZSConfig 并应用
    function applyAll() {
        ZSConfig.setValue("appearance/fontFamily", fontCombo.currentText)
        ZSConfig.setValue("appearance/fontSize", sizeSpin.value)
        ZSConfig.setValue("appearance/foreground", fgText.text)
        ZSConfig.setValue("appearance/background", bgText.text)
        ZSConfig.setValue("appearance/cursorColor", curText.text)
        ZSConfig.setValue("appearance/cursorShape", shapeCombo.currentIndex)
        ZSConfig.setValue("appearance/colorScheme", schemeCombo.currentIndex)
        ZSConfig.sync()
        ZSApplication.applySettings()
    }

    function loadFromConfig() {
        loading = true
        fontCombo.currentIndex = Math.max(0, fontCombo.find(cfgStr("appearance/fontFamily", defFont)))
        sizeSpin.value = cfgInt("appearance/fontSize", defSize)
        fgText.text = cfgStr("appearance/foreground", defFg)
        bgText.text = cfgStr("appearance/background", defBg)
        curText.text = cfgStr("appearance/cursorColor", defCursor)
        shapeCombo.currentIndex = cfgInt("appearance/cursorShape", defShape)
        schemeCombo.currentIndex = cfgInt("appearance/colorScheme", 0)
        loading = false
    }

    function applyScheme(idx) {
        if (idx === 1) {            // 浅色
            fgText.text = "#202020"; bgText.text = "#f5f5f5"; curText.text = "#202020"
        } else {                    // 深色（默认）
            fgText.text = "#c0c0c0"; bgText.text = "#000000"; curText.text = "#c0c0c0"
        }
    }

    Component.onCompleted: loadFromConfig()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label { text: "字体"; color: "#2b2f36"; font.bold: true; font.pointSize: 12 }

        RowLayout {
            Label { text: "字族"; color: "#6b7280"; Layout.preferredWidth: 70 }
            ComboBox {
                id: fontCombo
                Layout.fillWidth: true
                model: ZSApplication.fontFamilies()
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
            }
        }
        RowLayout {
            Label { text: "字号(px)"; color: "#6b7280"; Layout.preferredWidth: 70 }
            SpinBox {
                id: sizeSpin
                from: 8; to: 40; value: defSize
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d0d7e0"; Layout.topMargin: 6 }

        Label { text: "配色"; color: "#2b2f36"; font.bold: true; font.pointSize: 12 }

        RowLayout {
            Label { text: "方案"; color: "#6b7280"; Layout.preferredWidth: 70 }
            ComboBox {
                id: schemeCombo
                Layout.preferredWidth: 140
                model: ["深色", "浅色"]
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                onCurrentIndexChanged: if (!loading) applyScheme(currentIndex)
            }
        }
        RowLayout {
            Label { text: "前景"; color: "#6b7280"; Layout.preferredWidth: 70 }
            TextField { id: fgText; Layout.fillWidth: true; placeholderText: "#c0c0c0"; background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 } }
            Rectangle { width: 22; height: 22; color: fgText.text; border.color: "#aaa" }
        }
        RowLayout {
            Label { text: "背景"; color: "#6b7280"; Layout.preferredWidth: 70 }
            TextField { id: bgText; Layout.fillWidth: true; placeholderText: "#000000"; background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 } }
            Rectangle { width: 22; height: 22; color: bgText.text; border.color: "#aaa" }
        }
        RowLayout {
            Label { text: "光标"; color: "#6b7280"; Layout.preferredWidth: 70 }
            TextField { id: curText; Layout.fillWidth: true; placeholderText: "#c0c0c0"; background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 } }
            Rectangle { width: 22; height: 22; color: curText.text; border.color: "#aaa" }
        }
        RowLayout {
            Label { text: "光标形状"; color: "#6b7280"; Layout.preferredWidth: 70 }
            ComboBox { id: shapeCombo; Layout.preferredWidth: 140; model: ["块状", "下划线", "竖条"]; background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 } }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            Button {
                text: "应用"
                implicitWidth: 90
                background: Rectangle {
                    color: parent.hovered ? "#1f4f86" : "#2b6cb0"
                    radius: 4
                }
                contentItem: Label { text: "应用"; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter }
                onClicked: { applyAll(); dlg.close() }
            }
            Button {
                text: "取消"
                implicitWidth: 90
                background: Rectangle {
                    color: parent.hovered ? "#d3dbe6" : "#e3e8ee"
                    border.color: "#d0d7e0"
                    radius: 4
                }
                contentItem: Label { text: "取消"; color: "#2b2f36"; horizontalAlignment: Text.AlignHCenter }
                onClicked: dlg.close()
            }
        }
    }
}

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
    color: "#1c1e22"

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
        anchors.margins: 14
        spacing: 12

        Label { text: "字体"; color: "#c8ccd2"; font.bold: true }

        RowLayout {
            Label { text: "字族"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            ComboBox {
                id: fontCombo
                Layout.fillWidth: true
                model: ZSApplication.fontFamilies()
            }
        }
        RowLayout {
            Label { text: "字号(px)"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            SpinBox { id: sizeSpin; from: 8; to: 40; value: defSize }
        }

        Label { text: "配色"; color: "#c8ccd2"; font.bold: true; Layout.topMargin: 8 }

        RowLayout {
            Label { text: "方案"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            ComboBox {
                id: schemeCombo
                Layout.preferredWidth: 140
                model: ["深色", "浅色"]
                onCurrentIndexChanged: if (!loading) applyScheme(currentIndex)
            }
        }
        RowLayout {
            Label { text: "前景"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            TextField { id: fgText; Layout.fillWidth: true; placeholderText: "#c0c0c0" }
            Rectangle { width: 22; height: 22; color: fgText.text; border.color: "#444" }
        }
        RowLayout {
            Label { text: "背景"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            TextField { id: bgText; Layout.fillWidth: true; placeholderText: "#000000" }
            Rectangle { width: 22; height: 22; color: bgText.text; border.color: "#444" }
        }
        RowLayout {
            Label { text: "光标"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            TextField { id: curText; Layout.fillWidth: true; placeholderText: "#c0c0c0" }
            Rectangle { width: 22; height: 22; color: curText.text; border.color: "#444" }
        }
        RowLayout {
            Label { text: "光标形状"; color: "#8a8f98"; Layout.preferredWidth: 70 }
            ComboBox { id: shapeCombo; Layout.preferredWidth: 140; model: ["块状", "下划线", "竖条"] }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            Button {
                text: "应用"
                onClicked: { applyAll(); dlg.close() }
            }
            Button {
                text: "取消"
                onClicked: dlg.close()
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

Window {
    id: root
    width: 1024
    height: 680
    minimumWidth: 560
    minimumHeight: 400
    title: "ZosnTerm"
    visible: true

    // ---- MobaXTerm 风格调色板 ----
    readonly property color mobaBlue:     "#2b6cb0"
    readonly property color mobaBlueDark: "#1f4f86"
    readonly property color chromeBg:     "#eef1f5"
    readonly property color sideBg:       "#e3e8ee"
    readonly property color sideHover:    "#d3dbe6"
    readonly property color sideSel:      "#c3d4ec"
    readonly property color border:       "#d0d7e0"
    readonly property color textDark:     "#2b2f36"
    readonly property color textMuted:    "#7b828c"

    property bool sideCollapsed: false
    property var sessionModel: []

    // 把一组串口参数回填到各 ComboBox。
    function applyParams(port, baud, data, stop, parity, flow, codepage) {
        if (portCombo.find(port) >= 0) portCombo.currentIndex = portCombo.find(port);
        else portCombo.editText = port;
        baudCombo.currentIndex = baudCombo.indexOfValue(baud);
        dataCombo.currentIndex = dataCombo.indexOfValue(data);
        stopCombo.currentIndex = stopCombo.indexOfValue(stop);
        parityCombo.currentIndex = parityCombo.indexOfValue(parity);
        flowCombo.currentIndex = flowCombo.indexOfValue(flow);
        encCombo.currentIndex = encCombo.indexOfValue((codepage && codepage !== "") ? codepage : ZSApplication.defaultEncoding());
    }
    function refreshSessions() { sessionModel = ZSApplication.sessionList(); }

    Component.onCompleted: {
        portCombo.model = ZSApplication.availablePorts();
        encCombo.currentIndex = encCombo.indexOfValue(ZSApplication.defaultEncoding());
        refreshSessions();
        var last = ZSApplication.lastParams();
        if (last.port !== undefined && last.port !== "")
            applyParams(last.port, last.baudRate, last.dataBits,
                        last.stopBits, last.parity, last.flow, last.codepage);
    }

    Connections {
        target: ZSApplication
        function onConnectionParamsApplied(port, baud, data, stop, parity, flow, codepage) {
            applyParams(port, baud, data, stop, parity, flow, codepage);
        }
        function onConnectionStateChanged(c) { /* 各控件通过属性绑定响应 */ }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ============ 左侧会话栏 ============
        Rectangle {
            id: sidebar
            visible: !sideCollapsed
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            color: sideBg
            Rectangle { anchors.right: parent.right; width: 1; color: border }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: mobaBlue
                    Label {
                        text: "会话"
                        color: "#ffffff"
                        font.bold: true
                        font.pointSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 14
                    }
                }

                Button {
                    id: newSessBtn
                    text: "+ 新建串口会话"
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 10
                    Layout.bottomMargin: 6
                    background: Rectangle {
                        color: newSessBtn.hovered ? mobaBlueDark : mobaBlue
                        radius: 4
                    }
                    contentItem: Label {
                        text: newSessBtn.text
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        font.pointSize: 11
                    }
                    onClicked: {
                        applyParams(ZSApplication.availablePorts()[0] || "COM1", 9600, 8, 1, 0, 0);
                        portCombo.forceActiveFocus();
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ListView {
                        id: sessList
                        model: sessionModel
                        clip: true
                        spacing: 2
                        delegate: ItemDelegate {
                            width: sessList.width
                            height: 48
                            hoverEnabled: true
                            background: Rectangle {
                                color: (sessList.currentIndex === index) ? sideSel
                                     : (hovered ? sideHover : "transparent")
                            }
                            onClicked: {
                                sessList.currentIndex = index;
                                ZSApplication.openSession(modelData.name);
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 8
                                spacing: 10
                                Rectangle {
                                    width: 26; height: 26; radius: 5
                                    color: mobaBlue
                                    Label {
                                        text: "S"
                                        color: "#ffffff"
                                        font.bold: true
                                        anchors.centerIn: parent
                                        font.pointSize: 11
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Label {
                                        text: modelData.name
                                        color: textDark
                                        font.bold: true
                                        font.pointSize: 11
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        text: modelData.port + " · " + modelData.baudRate + " "
                                              + modelData.dataBits
                                              + (modelData.parity === 1 ? "O"
                                                 : modelData.parity === 2 ? "E"
                                                 : modelData.parity === 3 ? "M"
                                                 : modelData.parity === 4 ? "S" : "N")
                                              + (modelData.stopBits === 3 ? "1.5" : modelData.stopBits)
                                        color: textMuted
                                        font.pointSize: 9
                                    }
                                }
                            }
                        }
                    }
                }

                Button {
                    id: deleteBtn
                    text: "删除选中会话"
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 4
                    Layout.bottomMargin: 10
                    enabled: sessList.currentIndex >= 0
                    background: Rectangle {
                        color: (deleteBtn.hovered && deleteBtn.enabled) ? "#c0392b"
                             : (deleteBtn.enabled ? "#b9c2ce" : "#cfd6df")
                        radius: 4
                    }
                    contentItem: Label {
                        text: deleteBtn.text
                        color: deleteBtn.enabled ? "#ffffff" : "#8a909a"
                        horizontalAlignment: Text.AlignHCenter
                        font.pointSize: 11
                    }
                    onClicked: {
                        if (sessList.currentIndex >= 0) {
                            ZSApplication.deleteSession(sessionModel[sessList.currentIndex].name);
                            refreshSessions();
                            sessList.currentIndex = -1;
                        }
                    }
                }
            }
        }

        // ============ 右侧主区域 ============
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- 顶部参数工具栏 ---
            ToolBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                background: Rectangle {
                    color: "#ffffff"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    ToolButton {
                        id: sideToggle
                        width: 30; height: 30
                        onClicked: sideCollapsed = !sideCollapsed
                        background: Rectangle {
                            color: sideToggle.hovered ? sideHover : "transparent"
                            radius: 4
                        }
                        contentItem: Column {
                            spacing: 3
                            anchors.centerIn: parent
                            Repeater {
                                model: 3
                                Rectangle { width: 16; height: 2; color: textDark; radius: 1 }
                            }
                        }
                    }

                    ComboBox {
                        id: portCombo
                        editable: true
                        implicitWidth: 120
                    }
                    ComboBox {
                        id: baudCombo
                        implicitWidth: 86
                        valueRole: "v"
                        textRole: "t"
                        model: [
                            { t: "9600",   v: 9600 },
                            { t: "19200",  v: 19200 },
                            { t: "38400",  v: 38400 },
                            { t: "57600",  v: 57600 },
                            { t: "115200", v: 115200 },
                            { t: "460800", v: 460800 },
                            { t: "921600", v: 921600 }
                        ]
                        currentIndex: 0
                    }
                    ComboBox {
                        id: dataCombo
                        implicitWidth: 60
                        valueRole: "v"
                        textRole: "t"
                        model: [
                            { t: "5", v: 5 }, { t: "6", v: 6 },
                            { t: "7", v: 7 }, { t: "8", v: 8 }
                        ]
                        currentIndex: 3
                    }
                    ComboBox {
                        id: stopCombo
                        implicitWidth: 60
                        valueRole: "v"
                        textRole: "t"
                        model: [
                            { t: "1",   v: 1 },
                            { t: "1.5", v: 3 },
                            { t: "2",   v: 2 }
                        ]
                        currentIndex: 0
                    }
                    ComboBox {
                        id: parityCombo
                        implicitWidth: 68
                        valueRole: "v"
                        textRole: "t"
                        model: [
                            { t: "无",   v: 0 },
                            { t: "奇",   v: 1 },
                            { t: "偶",   v: 2 },
                            { t: "标记", v: 3 },
                            { t: "空格", v: 4 }
                        ]
                        currentIndex: 0
                    }
                    ComboBox {
                        id: flowCombo
                        implicitWidth: 86
                        valueRole: "v"
                        textRole: "t"
                        model: [
                            { t: "无",       v: 0 },
                            { t: "XON/XOFF", v: 1 },
                            { t: "RTS/CTS",  v: 2 }
                        ]
                        currentIndex: 0
                    }
                    ComboBox {
                        id: encCombo
                        implicitWidth: 104
                        textRole: "label"
                        valueRole: "cp"
                        model: ZSApplication.availableEncodings()
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        id: connectBtn
                        text: "连接"
                        enabled: !ZSApplication.connected
                        implicitWidth: 76
                        background: Rectangle {
                            color: (connectBtn.hovered && connectBtn.enabled) ? mobaBlueDark : mobaBlue
                            radius: 4
                        }
                        contentItem: Label {
                            text: connectBtn.text
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: 11
                        }
                        onClicked: ZSApplication.connectSerial(
                            portCombo.displayText,
                            baudCombo.currentValue,
                            dataCombo.currentValue,
                            stopCombo.currentValue,
                            parityCombo.currentValue,
                            flowCombo.currentValue,
                            encCombo.currentValue)
                    }
                    Button {
                        id: disconnectBtn
                        text: "断开"
                        enabled: ZSApplication.connected
                        implicitWidth: 76
                        background: Rectangle {
                            color: (disconnectBtn.hovered && disconnectBtn.enabled) ? "#a93226" : "#c0392b"
                            radius: 4
                        }
                        contentItem: Label {
                            text: disconnectBtn.text
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: 11
                        }
                        onClicked: ZSApplication.closeSerialSession()
                    }
                    Button {
                        id: settingsBtn
                        text: "设置"
                        implicitWidth: 76
                        background: Rectangle {
                            color: settingsBtn.hovered ? sideHover : "#f2f4f7"
                            border.color: border
                            radius: 4
                        }
                        contentItem: Label {
                            text: settingsBtn.text
                            color: textDark
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: 11
                        }
                        onClicked: settingsDlg.show()
                    }
                }
            }

            // --- 标签页栏 ---
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: "#dfe5ec"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 4

                    Rectangle {
                        height: 28
                        width: Math.max(140, tabLabel.implicitWidth + 28)
                        color: "#ffffff"
                        border.color: mobaBlue
                        border.width: 1
                        radius: 4
                        Rectangle { width: 3; height: parent.height; color: mobaBlue; radius: 2 }

                        Label {
                            id: tabLabel
                            text: ZSApplication.connectionSummary !== ""
                                  ? ZSApplication.connectionSummary : "未连接"
                            color: textDark
                            font.pointSize: 11
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 12
                        }
                    }

                    ToolButton {
                        text: "+"
                        width: 28; height: 28
                        enabled: false
                        opacity: 0.5
                        onClicked: {
                            applyParams(ZSApplication.availablePorts()[0] || "COM1", 9600, 8, 1, 0, 0);
                            portCombo.forceActiveFocus();
                        }
                        background: Rectangle {
                            color: parent.hovered ? sideHover : "transparent"
                            border.color: border
                            radius: 4
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // --- 终端视图 ---
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ZSTerminalView {
                    id: term
                    anchors.fill: parent
                    focus: true
                }

                // 右键菜单层：仅拦截右键，左键透传给终端（用于聚焦/输入）
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onPressed: {
                        var p = term.mapToItem(root.contentItem, mouse.x, mouse.y)
                        ctxMenu.popup(p.x, p.y)
                    }
                }

                // 查找栏（默认隐藏，从右键菜单"查找"唤出）
                Rectangle {
                    id: findBar
                    visible: false
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 34
                    color: "#f2f4f7"
                    border.color: root.border
                    z: 5
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 6
                        TextField {
                            id: findInput
                            Layout.fillWidth: true
                            placeholderText: "查找..."
                            onTextChanged: {
                                var n = ZSApplication.findText(text)
                                findStatus.text = (n > 0) ? ("匹配 " + n + " 处") : "无匹配"
                            }
                        }
                        Button { text: "上一个"; implicitWidth: 64; onClicked: ZSApplication.findPrev() }
                        Button { text: "下一个"; implicitWidth: 64; onClicked: ZSApplication.findNext() }
                        Label { id: findStatus; text: ""; font.pointSize: 10 }
                        Button {
                            text: "✕"; implicitWidth: 32
                            onClicked: { findBar.visible = false; ZSApplication.clearFind() }
                        }
                    }
                }
            }

            // --- 底部状态栏 ---
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                color: mobaBlue
                Label {
                    anchors.fill: parent
                    leftPadding: 10
                    rightPadding: 10
                    verticalAlignment: Qt.AlignVCenter
                    text: (ZSApplication.connectionSummary !== "" ? ZSApplication.connectionSummary + "    " : "")
                          + ZSApplication.statusMessage
                    color: "#ffffff"
                    font.pointSize: 10
                }
            }
        }
    }

    ZSSettingsDlg {
        id: settingsDlg
    }

    Menu {
        id: ctxMenu
        MenuItem { text: "全选"; onTriggered: ZSApplication.selectAll() }
        MenuItem { text: "复制"; onTriggered: ZSApplication.copySelection() }
        MenuItem { text: "粘贴"; onTriggered: ZSApplication.paste() }
        MenuItem { text: "清屏"; onTriggered: ZSApplication.clearScreen() }
        MenuSeparator { }
        MenuItem {
            text: "查找"
            onTriggered: { findBar.visible = true; findInput.forceActiveFocus() }
        }
    }
}

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

    property string activeSession: ""

    function refreshSessions() { sessionModel = ZSApplication.sessionList(); }

    // 会话名 -> sessionModel 下标（-1 表示未找到）。
    function sessionIndex(name) {
        for (var i = 0; i < sessionModel.length; ++i)
            if (sessionModel[i].name === name) return i
        return -1
    }

    // 统一切换会话：更新激活项、通知 C++、切到对应终端视图。
    function selectSession(name) {
        activeSession = name
        ZSApplication.setActiveSession(name)
        var idx = sessionIndex(name)
        if (idx >= 0) termStack.currentIndex = idx
    }

    // 打开新建会话对话框，用上次使用的参数作为默认值。
    function openNewSession() {
        var last = ZSApplication.lastParams()
        newSessionDlg.openWith(last.port || "",
                               last.baudRate || 9600,
                               last.dataBits || 8,
                               last.stopBits || 1,
                               last.parity || 0,
                               last.flow || 0,
                               last.codepage || ZSApplication.defaultEncoding())
    }

    Component.onCompleted: {
        refreshSessions()
        if (sessionModel.length > 0)
            selectSession(sessionModel[0].name)
    }

    Connections {
        target: ZSApplication
        function onConnectionStateChanged(c) { /* 选项卡连接圆点通过属性绑定响应 */ }
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
                    onClicked: openNewSession()
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
                                selectSession(modelData.name);
                                ZSApplication.connectSession(modelData.name);
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

            // --- 顶部工具条（串口参数已移至“新建会话”对话框；连接改用会话选项卡右键） ---
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

                    Label {
                        text: "串口终端"
                        color: textDark
                        font.bold: true
                        font.pointSize: 12
                        Layout.leftMargin: 4
                    }

                    Item { Layout.fillWidth: true }

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

            // --- 会话选项卡栏（左键选中，右键连接/断开/删除） ---
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: "#dfe5ec"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 4

                    Repeater {
                        model: sessionModel
                        Rectangle {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: Math.max(120, sessTabLabel.implicitWidth + 40)
                            color: (activeSession === modelData.name) ? "#ffffff" : "#e8edf3"
                            border.color: (activeSession === modelData.name) ? mobaBlue : border
                            border.width: 1
                            radius: 4

                            Rectangle {
                                width: 3; height: parent.height
                                color: (activeSession === modelData.name) ? mobaBlue : "#b8c2ce"
                                radius: 2
                            }
                            Label {
                                id: sessTabLabel
                                text: modelData.name
                                color: (activeSession === modelData.name) ? mobaBlue : textDark
                                font.bold: (activeSession === modelData.name)
                                font.pointSize: 11
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 11
                                anchors.right: parent.right
                                anchors.rightMargin: 16
                                elide: Text.ElideRight
                            }
                            Rectangle {  // 连接中的绿色指示点
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                width: 7; height: 7; radius: 4
                                color: "#27ae60"
                                visible: ZSApplication.connected && activeSession === modelData.name
                            }
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onClicked: function(mouse) {
                                    if (mouse.button === Qt.RightButton) {
                                        tabCtxMenu.sessionName = modelData.name
                                        var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                        tabCtxMenu.popup(p.x, p.y)
                                    } else {
                                        selectSession(modelData.name)
                                        sessList.currentIndex = index
                                    }
                                }
                            }
                        }
                    }

                    ToolButton {
                        text: "+"
                        width: 28; height: 28
                        ToolTip.text: "新建会话"
                        ToolTip.visible: hovered
                        onClicked: openNewSession()
                        background: Rectangle {
                            color: parent.hovered ? sideHover : "transparent"
                            border.color: border
                            radius: 4
                        }
                        contentItem: Label {
                            text: "+"; color: textDark
                            font.pointSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // --- 终端视图：每个会话一个独立终端（M13） ---
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                StackLayout {
                    id: termStack
                    anchors.fill: parent
                    currentIndex: sessionIndex(activeSession)

                    Repeater {
                        model: sessionModel
                        ZSTerminalView {
                            // 每个视图在完成后向 C++ 注册，绑定独立 PuTTY 引擎。
                            Component.onCompleted: {
                                ZSApplication.registerSessionView(modelData.name, this)
                            }
                        }
                    }
                }

                // 右键菜单层：仅拦截右键，左键透传给终端（用于聚焦/输入）
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onPressed: function(mouse) {
                        ZSApplication.cancelSelection()
                        var t = termStack.currentItem
                        var p = t ? t.mapToItem(root.contentItem, mouse.x, mouse.y)
                                  : mapToItem(root.contentItem, mouse.x, mouse.y)
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

    ZSNewSessionDlg {
        id: newSessionDlg
        onSaved: function(name) {
            refreshSessions();
            for (var i = 0; i < sessionModel.length; ++i) {
                if (sessionModel[i].name === name) {
                    sessList.currentIndex = i;
                    selectSession(name);
                    break;
                }
            }
        }
    }

    Menu {
        id: tabCtxMenu
        property string sessionName: ""
        MenuItem {
            text: "连接"
            enabled: !ZSApplication.connected
            onTriggered: ZSApplication.connectSession(tabCtxMenu.sessionName)
        }
        MenuItem {
            text: "断开"
            enabled: ZSApplication.connected
            onTriggered: ZSApplication.closeSession(tabCtxMenu.sessionName)
        }
        MenuSeparator { }
        MenuItem {
            text: "删除会话"
            onTriggered: {
                var removed = tabCtxMenu.sessionName
                ZSApplication.deleteSession(removed)
                refreshSessions()
                sessList.currentIndex = -1
                if (activeSession === removed) {
                    if (sessionModel.length > 0)
                        selectSession(sessionModel[0].name)
                    else
                        activeSession = ""
                }
            }
        }
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

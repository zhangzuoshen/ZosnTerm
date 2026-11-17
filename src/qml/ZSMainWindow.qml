import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

/* M15: layout rebuilt to match the Figma design (main-window-light, 1440x900).
 * All existing behaviour is preserved: multi-session independent terminals,
 * context menu (select/copy/paste/clear/find), drag selection, find-all
 * highlight, new-session/settings dialogs, and resize content retention. */
Window {
    id: root
    width: 1200
    height: 760
    minimumWidth: 720
    minimumHeight: 460
    title: "ZosnTerm"
    visible: true

    // ---- palette (Figma light theme) ----
    readonly property color accent:      "#2f6fed"   // 科技蓝
    readonly property color accentHover: "#2560d8"
    readonly property color accentSoft:  "#e8f0fe"
    readonly property color chromeBg:    "#ffffff"
    readonly property color sideBg:      "#f5f7fa"
    readonly property color sideHover:   "#eaeef5"
    readonly property color sideSel:     "#e3edff"
    readonly property color border:      "#dfe4ec"
    readonly property color textDark:    "#1f2329"
    readonly property color textMuted:   "#8a9099"
    readonly property color greenDot:    "#22c55e"
    readonly property color greyDot:     "#b6bdc7"
    readonly property color amberDot:    "#f59e0b"
    readonly property color terminalBg:  "#ffffff"

    property bool sideCollapsed: false
    property var sessionModel: []
    property string activeSession: ""

    // 输入栏开关（视觉 + 发送行为；时间戳为显示偏好，暂不参与引擎渲染）
    property bool showTimestamp: true
    property bool hexSend: false

    function refreshSessions() { sessionModel = ZSApplication.sessionList(); }

    function sessionIndex(name) {
        for (var i = 0; i < sessionModel.length; ++i)
            if (sessionModel[i].name === name) return i
        return -1
    }

    function sessionAt(name) {
        var i = sessionIndex(name)
        return i >= 0 ? sessionModel[i] : null
    }

    function selectSession(name) {
        activeSession = name
        ZSApplication.setActiveSession(name)
        var idx = sessionIndex(name)
        if (idx >= 0) termStack.currentIndex = idx
    }

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

    function connectActive() {
        if (activeSession !== "") ZSApplication.connectSession(activeSession)
    }
    function disconnectActive() {
        if (activeSession !== "") ZSApplication.closeSession(activeSession)
    }

    // parity 数值 -> 字符
    function parityText(p) {
        return p === 1 ? "O" : p === 2 ? "E" : p === 3 ? "M" : p === 4 ? "S" : "N"
    }

    Component.onCompleted: {
        refreshSessions()
        if (sessionModel.length > 0)
            selectSession(sessionModel[0].name)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ============ 标题栏 ============
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: chromeBg
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Rectangle {
                    width: 20; height: 20; radius: 4
                    color: accent
                    Label {
                        text: "X"; color: "#ffffff"; font.bold: true; font.pointSize: 11
                        anchors.centerIn: parent
                    }
                }
                Label { text: "ZosnTerm"; color: textDark; font.bold: true; font.pointSize: 11 }
                Label { text: "串口终端"; color: textMuted; font.pointSize: 9; Layout.leftMargin: 2 }
                Item { Layout.fillWidth: true }
                Label {
                    text: "●  " + (ZSApplication.connected ? "已连接" : "未连接")
                    color: ZSApplication.connected ? greenDot : textMuted
                    font.pointSize: 10
                }
            }
        }

        // ============ 工具栏 ============
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#fbfcfe"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                ToolButton {
                    id: sideToggle
                    width: 30; height: 30
                    onClicked: sideCollapsed = !sideCollapsed
                    background: Rectangle { color: sideToggle.hovered ? sideHover : "transparent"; radius: 4 }
                    contentItem: Column {
                        spacing: 3; anchors.centerIn: parent
                        Repeater { model: 3; Rectangle { width: 16; height: 2; color: textMuted; radius: 1 } }
                    }
                    ToolTip.text: "显示/隐藏会话栏"; ToolTip.visible: hovered
                }

                Rectangle { width: 1; height: 20; color: border; Layout.leftMargin: 4; Layout.rightMargin: 4 }

                ToolButton {
                    id: connBtn; width: 32; height: 30
                    enabled: activeSession !== "" && !ZSApplication.connected
                    onClicked: connectActive()
                    background: Rectangle { color: connBtn.hovered ? sideHover : "transparent"; radius: 4 }
                    contentItem: Label { text: "⇧"; color: connBtn.enabled ? accent : "#c2c8d0"; font.pointSize: 15;
                                          horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    ToolTip.text: "连接"; ToolTip.visible: hovered
                }
                ToolButton {
                    id: discBtn; width: 32; height: 30
                    enabled: ZSApplication.connected
                    onClicked: disconnectActive()
                    background: Rectangle { color: discBtn.hovered ? sideHover : "transparent"; radius: 4 }
                    contentItem: Label { text: "⏻"; color: discBtn.enabled ? textDark : "#c2c8d0"; font.pointSize: 14;
                                          horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    ToolTip.text: "断开"; ToolTip.visible: hovered
                }
                ToolButton {
                    id: clearBtn; width: 32; height: 30
                    onClicked: ZSApplication.clearScreen()
                    background: Rectangle { color: clearBtn.hovered ? sideHover : "transparent"; radius: 4 }
                    contentItem: Label { text: "⌫"; color: textDark; font.pointSize: 14;
                                          horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    ToolTip.text: "清屏"; ToolTip.visible: hovered
                }
                ToolButton {
                    id: findBtn; width: 32; height: 30
                    onClicked: { findBar.visible = true; findInput.forceActiveFocus() }
                    background: Rectangle { color: findBtn.hovered ? sideHover : "transparent"; radius: 4 }
                    contentItem: Label { text: "⌕"; color: textDark; font.pointSize: 16;
                                          horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    ToolTip.text: "查找"; ToolTip.visible: hovered
                }

                Rectangle { width: 1; height: 20; color: border; Layout.leftMargin: 6; Layout.rightMargin: 6 }

                Button {
                    id: tsBtn; text: "时间戳"; checkable: true; checked: true
                    implicitWidth: 56; implicitHeight: 24
                    onCheckedChanged: root.showTimestamp = checked
                    background: Rectangle { color: tsBtn.checked ? accent : "#f2f4f7"; radius: 4 }
                    contentItem: Label { text: tsBtn.text; color: tsBtn.checked ? "#ffffff" : textMuted;
                                         font.pointSize: 9; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    id: hexViewBtn; text: "HEX"; checkable: true
                    implicitWidth: 44; implicitHeight: 24
                    background: Rectangle { color: hexViewBtn.checked ? accent : "#f2f4f7"; radius: 4 }
                    contentItem: Label { text: "HEX"; color: hexViewBtn.checked ? "#ffffff" : textMuted;
                                         font.pointSize: 9; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    id: autoScrollBtn; text: "自动滚动"; checkable: true; checked: true
                    implicitWidth: 64; implicitHeight: 24
                    background: Rectangle { color: autoScrollBtn.checked ? accent : "#f2f4f7"; radius: 4 }
                    contentItem: Label { text: autoScrollBtn.text; color: autoScrollBtn.checked ? "#ffffff" : textMuted;
                                         font.pointSize: 9; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    id: settingsBtn; width: 32; height: 30
                    onClicked: settingsDlg.show()
                    background: Rectangle { color: settingsBtn.hovered ? sideHover : "transparent"; radius: 4 }
                    contentItem: Label { text: "⚙"; color: textDark; font.pointSize: 15;
                                          horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    ToolTip.text: "设置"; ToolTip.visible: hovered
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ============ 左侧会话栏 ============
            Rectangle {
                id: sidebar
                visible: !sideCollapsed
                Layout.preferredWidth: 264
                Layout.fillHeight: true
                color: sideBg
                Rectangle { anchors.right: parent.right; width: 1; color: border }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // 搜索框
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        Layout.margins: 10
                        color: "#ffffff"
                        border.color: border; border.width: 1; radius: 5
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                            Label { text: "⌕"; color: textMuted; font.pointSize: 12 }
                            TextField {
                                id: searchField
                                Layout.fillWidth: true
                                placeholderText: "搜索会话或端口"
                                background: Rectangle { color: "transparent" }
                                color: textDark; font.pointSize: 10
                                onTextChanged: sessList.model = filteredSessions()
                            }
                        }
                    }

                    Label {
                        text: "最近会话"
                        color: textMuted; font.pointSize: 9; font.bold: true
                        Layout.leftMargin: 14; Layout.topMargin: 2; Layout.bottomMargin: 4
                    }

                    function filteredSessions() {
                        var q = searchField.text.toLowerCase()
                        if (q === "") return sessionModel
                        var out = []
                        for (var i = 0; i < sessionModel.length; ++i) {
                            var s = sessionModel[i]
                            if (s.name.toLowerCase().indexOf(q) >= 0 ||
                                s.port.toLowerCase().indexOf(q) >= 0)
                                out.push(s)
                        }
                        return out
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
                                height: 52
                                hoverEnabled: true
                                background: Rectangle {
                                    color: (activeSession === modelData.name) ? sideSel
                                         : (hovered ? sideHover : "transparent")
                                }
                                onClicked: {
                                    selectSession(modelData.name)
                                    ZSApplication.connectSession(modelData.name)
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12; anchors.rightMargin: 10
                                    spacing: 9
                                    Rectangle {
                                        width: 26; height: 26; radius: 5
                                        color: accentSoft
                                        Label { text: "▤"; color: accent; font.pointSize: 12; anchors.centerIn: parent }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 1
                                        Label {
                                            text: modelData.name
                                            color: textDark; font.bold: true; font.pointSize: 10
                                            elide: Text.ElideRight; Layout.fillWidth: true
                                        }
                                        Label {
                                            text: modelData.port + " · " + modelData.baudRate + " " + modelData.dataBits
                                                  + parityText(modelData.parity) + (modelData.stopBits === 3 ? "1.5" : modelData.stopBits)
                                            color: textMuted; font.pointSize: 9
                                        }
                                    }
                                    Rectangle {
                                        width: 7; height: 7; radius: 4
                                        color: (ZSApplication.connected && activeSession === modelData.name) ? greenDot : greyDot
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }
                            }
                        }
                    }

                    // 会话分组标题（视觉分组，删除入口仍在选项卡右键）
                    Label {
                        text: "会话分组"
                        color: textMuted; font.pointSize: 9; font.bold: true
                        Layout.leftMargin: 14; Layout.topMargin: 6; Layout.bottomMargin: 4
                    }
                    Item { Layout.preferredHeight: 4 }

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 30; color: "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                            Label {
                                text: sessionModel.length + " 个会话 · "
                                      + (ZSApplication.connected ? "1 个已连接" : "0 个已连接")
                                color: textMuted; font.pointSize: 9
                            }
                            Item { Layout.fillWidth: true }
                            ToolButton {
                                id: delBtn; width: 24; height: 24
                                enabled: activeSession !== ""
                                onClicked: {
                                    var removed = activeSession
                                    ZSApplication.deleteSession(removed)
                                    refreshSessions()
                                    if (activeSession === removed) {
                                        if (sessionModel.length > 0) selectSession(sessionModel[0].name)
                                        else activeSession = ""
                                    }
                                }
                                background: Rectangle { color: delBtn.hovered ? sideHover : "transparent"; radius: 4 }
                                contentItem: Label { text: "🗑"; color: delBtn.enabled ? textMuted : "#c8ced6"; font.pointSize: 10;
                                                     horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                ToolTip.text: "删除当前会话"; ToolTip.visible: hovered
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

                // --- 会话选项卡栏 ---
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: "#f7f9fc"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4

                        Repeater {
                            model: sessionModel
                            Rectangle {
                                Layout.preferredHeight: 30
                                Layout.preferredWidth: Math.max(132, sessTabLabel.implicitWidth + 52)
                                color: (activeSession === modelData.name) ? "#ffffff" : "transparent"
                                border.color: (activeSession === modelData.name) ? border : "transparent"
                                border.width: 1
                                radius: 5

                                RowLayout {
                                    anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 6
                                    Rectangle {
                                        width: 7; height: 7; radius: 4
                                        color: (ZSApplication.connected && activeSession === modelData.name) ? greenDot
                                             : (modelData.port.indexOf("COM12") >= 0 ? amberDot : greyDot)
                                    }
                                    Label {
                                        id: sessTabLabel
                                        text: modelData.name
                                        color: (activeSession === modelData.name) ? accent : textDark
                                        font.bold: (activeSession === modelData.name)
                                        font.pointSize: 10
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: "×"; color: textMuted; font.pointSize: 12
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                var removed = modelData.name
                                                ZSApplication.deleteSession(removed)
                                                refreshSessions()
                                                if (activeSession === removed) {
                                                    if (sessionModel.length > 0) selectSession(sessionModel[0].name)
                                                    else activeSession = ""
                                                }
                                            }
                                        }
                                    }
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
                                        }
                                    }
                                }
                            }
                        }

                        ToolButton {
                            id: addTabBtn; text: "+"; width: 28; height: 28
                            onClicked: openNewSession()
                            background: Rectangle { color: addTabBtn.hovered ? sideHover : "transparent"; border.color: border; radius: 4 }
                            contentItem: Label { text: "+"; color: textDark; font.pointSize: 15;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "新建会话"; ToolTip.visible: hovered
                        }
                        Item { Layout.fillWidth: true }
                    }
                }

                // --- 终端视图：每会话一个独立终端（M13） ---
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
                                Component.onCompleted: {
                                    ZSApplication.registerSessionView(modelData.name, this)
                                }
                            }
                        }
                    }

                    // 右键菜单层：仅拦截右键，左键透传给终端
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

                    // 查找栏
                    Rectangle {
                        id: findBar
                        visible: false
                        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                        height: 36; color: "#f2f4f7"; border.color: root.border; z: 5
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                            TextField {
                                id: findInput
                                Layout.fillWidth: true
                                placeholderText: "查找..."
                                onTextChanged: {
                                    var n = ZSApplication.findText(text)
                                    findStatus.text = (n > 0) ? ("匹配 " + n + " 处") : "无匹配"
                                }
                            }
                            Button { text: "上一个"; implicitWidth: 62; onClicked: ZSApplication.findPrev() }
                            Button { text: "下一个"; implicitWidth: 62; onClicked: ZSApplication.findNext() }
                            Label { id: findStatus; text: ""; font.pointSize: 10 }
                            Button {
                                text: "✕"; implicitWidth: 32
                                onClicked: { findBar.visible = false; ZSApplication.clearFind() }
                            }
                        }
                    }
                }

                // --- 底部输入栏 ---
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    color: "#fbfcfe"
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 8

                        Button {
                            id: resetBtn; text: "复位"; implicitWidth: 52; implicitHeight: 28
                            onClicked: ZSApplication.clearScreen()
                            background: Rectangle { color: resetBtn.hovered ? sideHover : "#f2f4f7"; border.color: border; radius: 4 }
                            contentItem: Label { text: "复位"; color: textDark; font.pointSize: 10;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 30
                            color: "#ffffff"; border.color: border; border.width: 1; radius: 5
                            TextField {
                                id: inputField
                                anchors.fill: parent
                                leftPadding: 10; rightPadding: 10
                                placeholderText: "输入 AT 指令或自定义数据..."
                                background: Rectangle { color: "transparent" }
                                color: textDark; font.pointSize: 10
                                onAccepted: sendInput()
                            }
                        }

                        Button {
                            id: hexSendBtn; text: "HEX"; checkable: true; checked: root.hexSend
                            implicitWidth: 46; implicitHeight: 28
                            onCheckedChanged: root.hexSend = checked
                            background: Rectangle { color: hexSendBtn.checked ? accent : "#f2f4f7"; radius: 4 }
                            contentItem: Label { text: "HEX"; color: hexSendBtn.checked ? "#ffffff" : textMuted;
                                                 font.pointSize: 9; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Button {
                            id: sendBtn; text: "发送"; implicitWidth: 68; implicitHeight: 30
                            enabled: ZSApplication.connected && inputField.text.length > 0
                            onClicked: sendInput()
                            background: Rectangle { color: sendBtn.enabled ? (sendBtn.hovered ? accentHover : accent) : "#c9d3e0"; radius: 5 }
                            contentItem: Label { text: "发送"; color: "#ffffff"; font.pointSize: 10; font.bold: true;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }

                    function sendInput() {
                        if (inputField.text.length === 0) return
                        ZSApplication.sendToActiveSession(inputField.text, root.hexSend)
                        inputField.text = ""
                    }
                }

                // --- 状态栏 ---
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    color: "#f2f5f9"
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 14
                        RowLayout { spacing: 5
                            Rectangle { width: 6; height: 6; radius: 3; color: ZSApplication.connected ? greenDot : greyDot }
                            Label {
                                text: (function() {
                                    var s = sessionAt(activeSession)
                                    if (!s) return "未连接"
                                    return s.port + (ZSApplication.connected ? " 已连接" : " 未连接")
                                })()
                                color: textMuted; font.pointSize: 9
                            }
                        }
                        Label {
                            text: (function() {
                                var s = sessionAt(activeSession)
                                return s ? (s.baudRate + "  " + s.dataBits + parityText(s.parity)
                                            + (s.stopBits === 3 ? "1.5" : s.stopBits) + "  "
                                            + (s.codepage || "UTF-8")) : ""
                            })()
                            color: textMuted; font.pointSize: 9
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: ZSApplication.connectionSummary
                            color: textMuted; font.pointSize: 9; elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    ZSSettingsDlg { id: settingsDlg }

    ZSNewSessionDlg {
        id: newSessionDlg
        onSaved: function(name) {
            refreshSessions()
            for (var i = 0; i < sessionModel.length; ++i) {
                if (sessionModel[i].name === name) {
                    sessList.model = sessionModel
                    selectSession(name)
                    break
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
                if (activeSession === removed) {
                    if (sessionModel.length > 0) selectSession(sessionModel[0].name)
                    else activeSession = ""
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

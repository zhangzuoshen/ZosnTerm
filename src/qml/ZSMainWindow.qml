import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

/* M16: visual refinement against Figma main-window-light.
 * Keeps all behaviour: multi-session terminals, context menus, drag-select,
 * find-all highlight, new-session/settings dialogs, resize content retention. */
Window {
    id: root
    width: 1380
    height: 840
    minimumWidth: 900
    minimumHeight: 560
    title: "ZosnTerm"
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint   // 自绘标题栏，去掉系统双层标题

    // ---- palette (Figma light theme) ----
    readonly property color accent:      "#2f6fed"
    readonly property color accentHover: "#2560d8"
    readonly property color accentSoft:  "#eaf1ff"
    readonly property color chromeBg:    "#ffffff"
    readonly property color toolBg:      "#fbfcfe"
    readonly property color sideBg:      "#f5f7fa"
    readonly property color sideHover:   "#eaeef5"
    readonly property color sideSel:     "#e3edff"
    readonly property color border:      "#dfe4ec"
    readonly property color textDark:    "#1f2329"
    readonly property color textMuted:   "#8a9099"
    readonly property color greenDot:    "#22c55e"
    readonly property color greyDot:     "#b6bdc7"
    readonly property color amberDot:    "#f59e0b"

    property bool sideCollapsed: false
    property var sessionModel: []
    property string activeSession: ""

    property bool showTimestamp: true
    property bool hexView: false
    property bool autoScroll: true
    property bool hexSend: false

    // 状态栏统计（每秒刷新一次）
    property string statRx: "0 B"
    property string statTx: "0 B"
    property int statSeconds: 0

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
        newSessionDlg.openWith(ZSApplication.lastParams())
    }

    function connectActive() {
        if (activeSession !== "") ZSApplication.connectSession(activeSession)
    }
    function disconnectActive() {
        if (activeSession !== "") ZSApplication.closeSession(activeSession)
    }

    function parityText(p) {
        return p === 1 ? "O" : p === 2 ? "E" : p === 3 ? "M" : p === 4 ? "S" : "N"
    }
    function flowText(f) {
        return f === 1 ? "XON/XOFF" : f === 2 ? "RTS/CTS" : "None"
    }
    function groupOf(s) {
        return (s && s.group && s.group !== "") ? s.group : "默认分组"
    }
    function groupNames() {
        var out = []
        for (var i = 0; i < sessionModel.length; ++i) {
            var g = groupOf(sessionModel[i])
            if (out.indexOf(g) < 0) out.push(g)
        }
        return out
    }
    function sessionsOf(g) {
        var out = []
        for (var i = 0; i < sessionModel.length; ++i)
            if (groupOf(sessionModel[i]) === g) out.push(sessionModel[i])
        return out
    }
    function connectedCount() {
        var n = 0
        for (var i = 0; i < sessionModel.length; ++i)
            if (ZSApplication.isSessionConnected(sessionModel[i].name)) ++n
        return n
    }
    function fmtBytes(n) {
        if (n >= 1048576) return (n / 1048576).toFixed(1) + " MB"
        if (n >= 1024) return (n / 1024).toFixed(1) + " KB"
        return n + " B"
    }
    function fmtDuration(sec) {
        var h = Math.floor(sec / 3600)
        var m = Math.floor((sec % 3600) / 60)
        var s = sec % 60
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }
    function refreshStats() {
        statRx = fmtBytes(ZSApplication.rxBytes())
        statTx = fmtBytes(ZSApplication.txBytes())
        statSeconds = ZSApplication.connectionSeconds()
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.refreshStats()
    }

    Component.onCompleted: {
        refreshSessions()
        if (sessionModel.length > 0)
            selectSession(sessionModel[0].name)
    }

    // 无边框窗口：标题拖动
    MouseArea {
        id: titleDrag
        anchors.top: parent.top
        anchors.left: parent.left
        width: root.width - 132   // 右侧留给窗口按钮，避免拦截点击
        height: 38
        z: 1
        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton) root.startSystemMove()
        }
    }

    // 无边框窗口：边缘缩放手柄
    MouseArea {
        anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.right: parent.right
        width: 5; cursorShape: Qt.SizeHorCursor; z: 4
        property point origin: Qt.point(0, 0); property real startW: 0
        onPressed: function(mouse) { origin = mapToGlobal(Qt.point(mouse.x, mouse.y)); startW = root.width }
        onPositionChanged: function(mouse) {
            if (pressed) root.width = Math.max(root.minimumWidth, startW + mapToGlobal(Qt.point(mouse.x, mouse.y)).x - origin.x)
        }
    }
    MouseArea {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 5; cursorShape: Qt.SizeVerCursor; z: 4
        property point origin: Qt.point(0, 0); property real startH: 0
        onPressed: function(mouse) { origin = mapToGlobal(Qt.point(mouse.x, mouse.y)); startH = root.height }
        onPositionChanged: function(mouse) {
            if (pressed) root.height = Math.max(root.minimumHeight, startH + mapToGlobal(Qt.point(mouse.x, mouse.y)).y - origin.y)
        }
    }
    MouseArea {
        anchors.right: parent.right; anchors.bottom: parent.bottom
        width: 12; height: 12; cursorShape: Qt.SizeFDiagCursor; z: 5
        property point origin: Qt.point(0, 0); property real startW: 0; property real startH: 0
        onPressed: function(mouse) {
            origin = mapToGlobal(Qt.point(mouse.x, mouse.y)); startW = root.width; startH = root.height
        }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            var g = mapToGlobal(Qt.point(mouse.x, mouse.y))
            root.width = Math.max(root.minimumWidth, startW + g.x - origin.x)
            root.height = Math.max(root.minimumHeight, startH + g.y - origin.y)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ============ 标题栏（自绘，无系统标题栏） ============
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: chromeBg
            z: 2
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 0
                spacing: 8

                Rectangle {
                    width: 18; height: 18; radius: 4; color: accent
                    Label { text: "X"; color: "#ffffff"; font.bold: true; font.pointSize: 10; anchors.centerIn: parent }
                }
                Label { text: "ZosnTerm"; color: textDark; font.bold: true; font.pointSize: 11 }
                Label { text: "串口终端"; color: textMuted; font.pointSize: 9; Layout.leftMargin: 1 }
                Item { Layout.fillWidth: true }

                // 窗口按钮：最小化 / 最大化 / 关闭
                ToolButton {
                    id: minBtn; width: 40; height: 38
                    onClicked: root.showMinimized()
                    background: Rectangle { color: minBtn.hovered ? "#eef1f5" : "transparent" }
                    contentItem: Label { text: "─"; color: textDark; font.pointSize: 12;
                                         horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    ToolTip.text: "最小化"; ToolTip.visible: hovered
                }
                ToolButton {
                    id: maxBtn; width: 40; height: 38
                    onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                    background: Rectangle { color: maxBtn.hovered ? "#eef1f5" : "transparent" }
                    contentItem: Label {
                        text: root.visibility === Window.Maximized ? "❐" : "□"
                        color: textDark; font.pointSize: 12
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    ToolTip.text: "最大化/还原"; ToolTip.visible: hovered
                }
                ToolButton {
                    id: closeBtn; width: 44; height: 38
                    onClicked: root.close()
                    background: Rectangle { color: closeBtn.hovered ? "#e81123" : "transparent" }
                    contentItem: Label {
                        text: "×"; color: closeBtn.hovered ? "#ffffff" : textDark; font.pointSize: 15
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    ToolTip.text: "关闭"; ToolTip.visible: hovered
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
                Layout.preferredWidth: 252
                Layout.fillHeight: true
                color: sideBg
                Rectangle { anchors.right: parent.right; width: 1; color: border }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // 会话标题行（+ / 搜索）
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        Layout.leftMargin: 12; Layout.rightMargin: 8
                        Label { text: "会话"; color: textDark; font.bold: true; font.pointSize: 11 }
                        Item { Layout.fillWidth: true }
                        ToolButton {
                            id: sidebarAdd; width: 22; height: 22
                            onClicked: openNewSession()
                            background: Rectangle { color: sidebarAdd.hovered ? sideHover : "transparent"; border.color: border; radius: 4 }
                            contentItem: Label { text: "+"; color: textDark; font.pointSize: 12;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        ToolButton {
                            id: sidebarFind; width: 22; height: 22
                            onClicked: searchField.forceActiveFocus()
                            background: Rectangle { color: sidebarFind.hovered ? sideHover : "transparent"; border.color: border; radius: 4 }
                            contentItem: Label { text: "⌕"; color: textDark; font.pointSize: 13;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }

                    // 搜索框
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        Layout.leftMargin: 10; Layout.rightMargin: 10; Layout.bottomMargin: 8
                        color: "#ffffff"
                        border.color: border; border.width: 1; radius: 5
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                            Label { text: "⌕"; color: textMuted; font.pointSize: 11 }
                            TextField {
                                id: searchField
                                Layout.fillWidth: true
                                placeholderText: "搜索会话或端口"
                                background: Rectangle { color: "transparent" }
                                color: textDark; font.pointSize: 9
                                onTextChanged: sessList.model = filteredSessions()
                            }
                        }
                    }

                    Label {
                        text: "最近会话"
                        color: textMuted; font.pointSize: 8; font.bold: true
                        Layout.leftMargin: 12; Layout.bottomMargin: 4
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
                        Layout.preferredHeight: 168
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        ListView {
                            id: sessList
                            model: sessionModel
                            clip: true
                            spacing: 1
                            delegate: ItemDelegate {
                                width: sessList.width
                                height: 46
                                hoverEnabled: true
                                background: Rectangle {
                                    color: (activeSession === modelData.name) ? sideSel
                                         : (hovered ? sideHover : "transparent")
                                }
                                onClicked: {
                                    selectSession(modelData.name)
                                    ZSApplication.connectSession(modelData.name)
                                }
                                Rectangle {   // 选中蓝条
                                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                                    width: 3; color: accent
                                    visible: activeSession === modelData.name
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 14; anchors.rightMargin: 10; spacing: 9
                                    Rectangle {
                                        width: 22; height: 22; radius: 4
                                        color: accentSoft
                                        Label { text: "▤"; color: accent; font.pointSize: 10; anchors.centerIn: parent }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 0
                                        Label {
                                            text: modelData.name
                                            color: textDark; font.bold: true; font.pointSize: 9
                                            elide: Text.ElideRight; Layout.fillWidth: true
                                        }
                                        Label {
                                            text: modelData.baudRate + " " + modelData.dataBits + parityText(modelData.parity)
                                                  + (modelData.stopBits === 3 ? "1.5" : modelData.stopBits)
                                                  + " · " + (ZSApplication.isSessionConnected(modelData.name) ? "已连接" : "未连接")
                                            color: textMuted; font.pointSize: 8
                                        }
                                    }
                                    Rectangle {
                                        width: 6; height: 6; radius: 3
                                        color: ZSApplication.isSessionConnected(modelData.name) ? greenDot : greyDot
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        text: "会话分组"
                        color: textMuted; font.pointSize: 8; font.bold: true
                        Layout.leftMargin: 12; Layout.topMargin: 8; Layout.bottomMargin: 4
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        ColumnLayout {
                            width: parent.width
                            spacing: 1
                            Repeater {
                                model: groupNames()
                                delegate: ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    property bool expanded: true
                                    property string grp: modelData
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 28
                                        Layout.leftMargin: 8; Layout.rightMargin: 12
                                        MouseArea {
                                            width: 14; height: 28
                                            onClicked: expanded = !expanded
                                            Label {
                                                anchors.centerIn: parent
                                                text: expanded ? "⌄" : "›"; color: textMuted; font.pointSize: 11
                                            }
                                        }
                                        Label { text: grp; color: textDark; font.pointSize: 9; Layout.fillWidth: true }
                                        Label { text: sessionsOf(grp).length; color: textMuted; font.pointSize: 8 }
                                    }
                                    Repeater {
                                        model: sessionsOf(grp)
                                        delegate: ItemDelegate {
                                            visible: expanded
                                            width: parent.width
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 28
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
                                                anchors.leftMargin: 28; anchors.rightMargin: 12; spacing: 8
                                                Rectangle { width: 10; height: 10; radius: 2; border.color: textMuted; border.width: 1; color: "transparent" }
                                                Label {
                                                    text: modelData.name; color: textDark; font.pointSize: 9
                                                    elide: Text.ElideRight; Layout.fillWidth: true
                                                }
                                                Rectangle {
                                                    width: 6; height: 6; radius: 3
                                                    color: ZSApplication.isSessionConnected(modelData.name) ? greenDot : greyDot
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 28; color: "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 10
                            Label {
                                text: sessionModel.length + " 个会话 · " + connectedCount() + " 个已连接"
                                color: textMuted; font.pointSize: 8
                            }
                            Item { Layout.fillWidth: true }
                            ToolButton {
                                id: delBtn; width: 22; height: 22
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
                                contentItem: Label { text: "⟳"; color: delBtn.enabled ? textMuted : "#c8ced6"; font.pointSize: 11;
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

                // ============ 工具栏 ============
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: toolBg
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 10
                        spacing: 4

                        // 蓝色新建按钮
                        ToolButton {
                            id: newBtn; width: 24; height: 24
                            onClicked: openNewSession()
                            background: Rectangle { color: accent; radius: 4 }
                            contentItem: Label { text: "+"; color: "#ffffff"; font.bold: true; font.pointSize: 14;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "新建会话"; ToolTip.visible: hovered
                        }

                        Rectangle { width: 1; height: 18; color: border; Layout.leftMargin: 6; Layout.rightMargin: 4 }

                        ToolButton {
                            id: connBtn; width: 28; height: 26
                            enabled: activeSession !== "" && !ZSApplication.connected
                            onClicked: connectActive()
                            background: Rectangle { color: connBtn.hovered ? sideHover : "transparent"; radius: 4 }
                            contentItem: Label { text: "⇄"; color: connBtn.enabled ? accent : "#c2c8d0"; font.pointSize: 15;
                                                  horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "连接"; ToolTip.visible: hovered
                        }
                        ToolButton {
                            id: discBtn; width: 28; height: 26
                            enabled: ZSApplication.connected
                            onClicked: disconnectActive()
                            background: Rectangle { color: discBtn.hovered ? sideHover : "transparent"; radius: 4 }
                            contentItem: Label { text: "⤬"; color: discBtn.enabled ? textDark : "#c2c8d0"; font.pointSize: 14;
                                                  horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "断开"; ToolTip.visible: hovered
                        }
                        ToolButton {
                            id: clearBtn; width: 28; height: 26
                            onClicked: ZSApplication.clearScreen()
                            background: Rectangle { color: clearBtn.hovered ? sideHover : "transparent"; radius: 4 }
                            contentItem: Label { text: "⌫"; color: textDark; font.pointSize: 14;
                                                  horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "清屏"; ToolTip.visible: hovered
                        }
                        ToolButton {
                            id: docBtn; width: 28; height: 26
                            onClicked: Qt.openUrlExternally("https://github.com/zhangzuoshen/ZosnTerm")
                            background: Rectangle { color: docBtn.hovered ? sideHover : "transparent"; radius: 4 }
                            contentItem: Label { text: "▤"; color: textDark; font.pointSize: 14;
                                                  horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "文档"; ToolTip.visible: hovered
                        }

                        Rectangle { width: 1; height: 18; color: border; Layout.leftMargin: 6; Layout.rightMargin: 6 }

                        Button {
                            id: tsBtn; text: "时间戳"; checkable: true; checked: root.showTimestamp
                            implicitWidth: 58; implicitHeight: 24
                            onCheckedChanged: root.showTimestamp = checked
                            background: Rectangle { color: tsBtn.checked ? accentSoft : "#ffffff"
                                                    border.color: tsBtn.checked ? accent : border; border.width: 1; radius: 4 }
                            contentItem: Label { text: tsBtn.text; color: tsBtn.checked ? accent : textMuted; font.pointSize: 9;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            id: hexViewBtn; text: "HEX"; checkable: true; checked: root.hexView
                            implicitWidth: 44; implicitHeight: 24
                            onCheckedChanged: root.hexView = checked
                            background: Rectangle { color: hexViewBtn.checked ? accentSoft : "#ffffff"
                                                    border.color: hexViewBtn.checked ? accent : border; border.width: 1; radius: 4 }
                            contentItem: Label { text: "HEX"; color: hexViewBtn.checked ? accent : textMuted; font.pointSize: 9;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            id: scrollBtn; text: "自动滚动"; checkable: true; checked: root.autoScroll
                            implicitWidth: 66; implicitHeight: 24
                            onCheckedChanged: root.autoScroll = checked
                            background: Rectangle { color: scrollBtn.checked ? accentSoft : "#ffffff"
                                                    border.color: scrollBtn.checked ? accent : border; border.width: 1; radius: 4 }
                            contentItem: Label { text: scrollBtn.text; color: scrollBtn.checked ? accent : textMuted; font.pointSize: 9;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Item { Layout.fillWidth: true }

                        ToolButton {
                            id: findBtn; width: 28; height: 26
                            onClicked: { findBar.visible = true; findInput.forceActiveFocus() }
                            background: Rectangle { color: findBtn.hovered ? sideHover : "transparent"; radius: 4 }
                            contentItem: Label { text: "⌕"; color: textDark; font.pointSize: 17;
                                                  horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "查找"; ToolTip.visible: hovered
                        }
                        ToolButton {
                            id: settingsBtn; width: 28; height: 26
                            onClicked: settingsDlg.show()
                            background: Rectangle { color: settingsBtn.hovered ? sideHover : "transparent"; radius: 4 }
                            contentItem: Label { text: "⚙"; color: textDark; font.pointSize: 14;
                                                  horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            ToolTip.text: "设置"; ToolTip.visible: hovered
                        }
                    }
                }

                // --- 选项卡栏：下划线风格 ---
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: "#ffffff"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 0

                        Repeater {
                            model: sessionModel
                            Rectangle {
                                Layout.preferredHeight: 36
                                Layout.preferredWidth: Math.max(136, sessTabLabel.implicitWidth + 52)
                                color: "transparent"

                                Rectangle {   // 选中下划线
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left; anchors.right: parent.right
                                    height: 2; color: accent
                                    visible: activeSession === modelData.name
                                }
                                RowLayout {
                                    anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 8; spacing: 7
                                    Rectangle {
                                        width: 6; height: 6; radius: 3
                                        color: ZSApplication.isSessionConnected(modelData.name) ? greenDot
                                             : (modelData.port.indexOf("COM12") >= 0 ? amberDot : greyDot)
                                    }
                                    Label {
                                        id: sessTabLabel
                                        text: modelData.name
                                        color: (activeSession === modelData.name) ? accent : textMuted
                                        font.bold: (activeSession === modelData.name)
                                        font.pointSize: 9
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: "×"; color: textMuted; font.pointSize: 11
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
                            id: addTabBtn; text: "+"; width: 28; height: 26
                            onClicked: openNewSession()
                            background: Rectangle { color: addTabBtn.hovered ? sideHover : "transparent"; border.color: border; radius: 4 }
                            contentItem: Label { text: "+"; color: textDark; font.pointSize: 14;
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

                    // 跳到最新
                    Rectangle {
                        anchors.right: parent.right; anchors.bottom: parent.bottom
                        anchors.rightMargin: 16; anchors.bottomMargin: 14
                        width: 84; height: 26; radius: 13
                        color: "#ffffff"; border.color: border
                        z: 4
                        RowLayout {
                            anchors.centerIn: parent; spacing: 4
                            Label { text: "↑"; color: accent; font.pointSize: 10 }
                            Label { text: "跳到最新"; color: textMuted; font.pointSize: 8 }
                        }
                        MouseArea { anchors.fill: parent; onClicked: ZSApplication.scrollToBottom() }
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
                    Layout.preferredHeight: 46
                    color: toolBg
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 8
                        Label { text: "发送"; color: textMuted; font.pointSize: 9 }

                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 28
                            color: "#ffffff"; border.color: border; border.width: 1; radius: 5
                            TextField {
                                id: inputField
                                anchors.fill: parent
                                leftPadding: 10; rightPadding: 10
                                placeholderText: "输入 AT 指令或自定义命令..."
                                background: Rectangle { color: "transparent" }
                                color: textDark; font.pointSize: 9
                                onAccepted: sendInput()
                            }
                        }

                        Button {
                            id: hexSendBtn; text: "HEX"; checkable: true; checked: root.hexSend
                            implicitWidth: 38; implicitHeight: 26
                            onCheckedChanged: root.hexSend = checked
                            background: Rectangle { color: hexSendBtn.checked ? accentSoft : "#ffffff"
                                                    border.color: hexSendBtn.checked ? accent : border; border.width: 1; radius: 4 }
                            contentItem: Label { text: "HEX"; color: hexSendBtn.checked ? accent : textMuted; font.pointSize: 9;
                                                 horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Button {
                            id: sendBtn; text: "发送"; implicitWidth: 62; implicitHeight: 28
                            enabled: ZSApplication.connected && inputField.text.length > 0
                            onClicked: sendInput()
                            background: Rectangle { color: sendBtn.enabled ? (sendBtn.hovered ? accentHover : accent) : "#c9d3e0"; radius: 5 }
                            contentItem: Label { text: "发送"; color: "#ffffff"; font.pointSize: 9; font.bold: true;
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
                    Layout.preferredHeight: 26
                    color: "#f2f5f9"
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: border }

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 14
                        RowLayout { spacing: 5
                            Rectangle { width: 6; height: 6; radius: 3; color: ZSApplication.connected ? greenDot : greyDot }
                            Label {
                                text: (function() {
                                    var s = sessionAt(activeSession)
                                    if (!s) return "未连接"
                                    return s.port + (ZSApplication.connected ? " 已连接" : " 未连接")
                                })()
                                color: textMuted; font.pointSize: 8
                            }
                        }
                        Label {
                            text: (function() {
                                var s = sessionAt(activeSession)
                                return s ? String(s.baudRate) : ""
                            })()
                            color: textMuted; font.pointSize: 8
                        }
                        Label {
                            text: (function() {
                                var s = sessionAt(activeSession)
                                return s ? (s.dataBits + parityText(s.parity) + (s.stopBits === 3 ? "1.5" : s.stopBits)) : ""
                            })()
                            color: textMuted; font.pointSize: 8
                        }
                        Label {
                            text: (function() {
                                var s = sessionAt(activeSession)
                                return s ? flowText(s.flow) : ""
                            })()
                            color: textMuted; font.pointSize: 8
                        }
                        Label {
                            text: (function() {
                                var s = sessionAt(activeSession)
                                return s ? (s.codepage || "UTF-8") : ""
                            })()
                            color: textMuted; font.pointSize: 8
                        }
                        Item { Layout.fillWidth: true }
                        Label { text: "RX " + statRx; color: textMuted; font.pointSize: 8 }
                        Label { text: "TX " + statTx; color: textMuted; font.pointSize: 8 }
                        Label { text: fmtDuration(statSeconds); color: textMuted; font.pointSize: 8 }
                    }
                }
            }
        }
    }

    ZSSettingsDlg { id: settingsDlg }

    ZSNewSessionDlg {
        id: newSessionDlg
        onSaved: function(name, connectNow) {
            refreshSessions()
            for (var i = 0; i < sessionModel.length; ++i) {
                if (sessionModel[i].name === name) {
                    sessList.model = sessionModel
                    selectSession(name)
                    break
                }
            }
            if (connectNow) ZSApplication.connectSession(name)
        }
    }

    Menu {
        id: tabCtxMenu
        property string sessionName: ""
        MenuItem {
            text: "连接"
            enabled: !ZSApplication.isSessionConnected(tabCtxMenu.sessionName)
            onTriggered: ZSApplication.connectSession(tabCtxMenu.sessionName)
        }
        MenuItem {
            text: "断开"
            enabled: ZSApplication.isSessionConnected(tabCtxMenu.sessionName)
            onTriggered: ZSApplication.closeSession(tabCtxMenu.sessionName)
        }
        MenuItem {
            text: "编辑会话"
            onTriggered: newSessionDlg.openWith(ZSApplication.sessionConfig(tabCtxMenu.sessionName))
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ZosnTerm 1.0

Window {
    id: dlg
    title: editing ? "编辑串口会话" : "新建串口会话"
    width: 760
    height: 580
    minimumWidth: 700
    minimumHeight: 540
    modality: Qt.ApplicationModal
    color: "#eef1f5"

    signal saved(string name, bool connectNow)

    property bool editing: false
    property string tipText: ""
    property bool tipOk: true

    function exists(n) {
        var names = ZSApplication.sessionNames();
        for (var i = 0; i < names.length; ++i)
            if (names[i] === n) return true;
        return false;
    }

    function urlToPath(u) {
        var s = u.toString();
        if (s.indexOf("file:///") === 0) return s.substring(8);
        if (s.indexOf("file://") === 0) return s.substring(7);
        return s;
    }

    function refreshPorts() {
        var ports = ZSApplication.availablePorts();
        portCombo.model = ports;
        return ports;
    }

    // cfg: QVariantMap（来自 lastParams / sessionConfig），空对象表示全新会话。
    function openWith(cfg) {
        cfg = cfg || {};
        editing = !!(cfg.name && cfg.name !== "");
        nameField.text = cfg.name || "";
        hint.text = "";
        tipText = "";

        var ports = refreshPorts();
        var p = cfg.port || "";
        if (p !== "" && portCombo.find(p) >= 0)
            portCombo.currentIndex = portCombo.find(p);
        else
            portCombo.editText = p || (ports.length ? ports[0] : "COM1");

        baudCombo.currentIndex = Math.max(0, baudCombo.indexOfValue(Number(cfg.baudRate) || 9600));
        dataCombo.currentIndex = Math.max(0, dataCombo.indexOfValue(Number(cfg.dataBits) || 8));
        stopCombo.currentIndex = Math.max(0, stopCombo.indexOfValue(Number(cfg.stopBits) || 1));
        parityCombo.currentIndex = Math.max(0, parityCombo.indexOfValue(Number(cfg.parity) || 0));
        flowCombo.currentIndex = Math.max(0, flowCombo.indexOfValue(Number(cfg.flow) || 0));
        var cp = cfg.codepage || ZSApplication.defaultEncoding();
        var ei = encCombo.indexOfValue(cp);
        encCombo.currentIndex = ei >= 0 ? ei : 0;

        tsCheck.checked        = cfg.showTimestamp !== undefined ? !!cfg.showTimestamp : false;
        hexCheck.checked       = cfg.hexDisplay !== undefined ? !!cfg.hexDisplay : false;
        echoCheck.checked      = cfg.localEcho !== undefined ? !!cfg.localEcho : false;
        scrollCheck.checked    = cfg.autoScroll !== undefined ? !!cfg.autoScroll : true;
        reconnectCheck.checked = !!cfg.autoReconnect;
        crlfCheck.checked      = !!cfg.crlf;
        logCheck.checked       = !!cfg.logEnabled;
        logPathField.text      = cfg.logPath || ZSApplication.defaultLogPath();
        groupField.text        = cfg.group || "";

        dlg.show();
        nameField.forceActiveFocus();
    }

    function extras() {
        return {
            showTimestamp: tsCheck.checked,
            hexDisplay: hexCheck.checked,
            localEcho: echoCheck.checked,
            autoScroll: scrollCheck.checked,
            autoReconnect: reconnectCheck.checked,
            crlf: crlfCheck.checked,
            logEnabled: logCheck.checked,
            logPath: logPathField.text.trim(),
            group: groupField.text.trim()
        };
    }

    function doTest() {
        var port = portCombo.displayText.trim();
        var err = ZSApplication.testSerial(port, baudCombo.currentValue,
                                           dataCombo.currentValue, stopCombo.currentValue,
                                           parityCombo.currentValue, flowCombo.currentValue);
        tipOk = (err === "");
        tipText = tipOk ? ("测试通过：" + port + " 可正常打开") : err;
    }

    function doSave(connectNow) {
        var n = nameField.text.trim();
        if (n === "") {
            hint.text = "请输入会话名";
            hint.color = "#c0392b";
            return;
        }
        var port = portCombo.displayText.trim();
        if (port === "") {
            hint.text = "请输入或选择串口号";
            hint.color = "#c0392b";
            return;
        }
        ZSApplication.saveSession(n, port,
                                  baudCombo.currentValue, dataCombo.currentValue,
                                  stopCombo.currentValue, parityCombo.currentValue,
                                  flowCombo.currentValue, encCombo.currentValue,
                                  extras());
        saved(n, connectNow);
        dlg.close();
    }

    FolderDialog {
        id: folderDlg
        title: "选择日志目录"
        onAccepted: logPathField.text = dlg.urlToPath(selectedFolder)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 20; color: "#2f6fed"; radius: 2 }
            Label {
                text: dlg.title
                color: "#2b2f36"
                font.bold: true
                font.pointSize: 15
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // ---------- 左：基本参数 ----------
            ColumnLayout {
                Layout.preferredWidth: 350
                Layout.fillHeight: true
                spacing: 8

                Label { text: "基本参数"; color: "#2f6fed"; font.bold: true; font.pointSize: 12; Layout.bottomMargin: 2 }

                RowLayout {
                    Label { text: "会话名"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    TextField {
                        id: nameField
                        Layout.fillWidth: true
                        placeholderText: "例如：COM4-调试"
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                        onTextChanged: {
                            var t = text.trim();
                            hint.color = "#b7791f";
                            hint.text = (dlg.exists(t) && t !== "") ? "同名会话已存在，保存将覆盖" : "";
                        }
                    }
                }
                RowLayout {
                    Label { text: "串口号"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    ComboBox {
                        id: portCombo
                        editable: true
                        Layout.fillWidth: true
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                    Button {
                        text: "刷新"
                        implicitWidth: 56
                        onClicked: {
                            var ports = dlg.refreshPorts();
                            if (ports.length) portCombo.editText = ports[0];
                            tipText = "已刷新串口列表（" + ports.length + " 个）";
                            tipOk = true;
                        }
                        background: Rectangle { color: parent.hovered ? "#d3dbe6" : "#e3e8ee"; border.color: "#d0d7e0"; radius: 4 }
                        contentItem: Label { text: parent.text; color: "#2b2f36"; horizontalAlignment: Text.AlignHCenter }
                    }
                }
                RowLayout {
                    Label { text: "波特率"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    ComboBox {
                        id: baudCombo
                        Layout.fillWidth: true
                        valueRole: "v"; textRole: "t"
                        model: [
                            { t: "9600",   v: 9600 },  { t: "19200",  v: 19200 },
                            { t: "38400",  v: 38400 }, { t: "57600",  v: 57600 },
                            { t: "115200", v: 115200 },{ t: "460800", v: 460800 },
                            { t: "921600", v: 921600 }
                        ]
                        currentIndex: 0
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                }
                RowLayout {
                    Label { text: "数据位"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    ComboBox {
                        id: dataCombo
                        Layout.fillWidth: true
                        valueRole: "v"; textRole: "t"
                        model: [ { t: "5", v: 5 }, { t: "6", v: 6 }, { t: "7", v: 7 }, { t: "8", v: 8 } ]
                        currentIndex: 3
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                    Label { text: "停止位"; color: "#6b7280" }
                    ComboBox {
                        id: stopCombo
                        Layout.preferredWidth: 80
                        valueRole: "v"; textRole: "t"
                        model: [ { t: "1", v: 1 }, { t: "1.5", v: 3 }, { t: "2", v: 2 } ]
                        currentIndex: 0
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                }
                RowLayout {
                    Label { text: "校验位"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    ComboBox {
                        id: parityCombo
                        Layout.fillWidth: true
                        valueRole: "v"; textRole: "t"
                        model: [
                            { t: "无",   v: 0 }, { t: "奇",   v: 1 }, { t: "偶",   v: 2 },
                            { t: "标记", v: 3 }, { t: "空格", v: 4 }
                        ]
                        currentIndex: 0
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                    Label { text: "流控"; color: "#6b7280" }
                    ComboBox {
                        id: flowCombo
                        Layout.preferredWidth: 96
                        valueRole: "v"; textRole: "t"
                        model: [ { t: "无", v: 0 }, { t: "XON/XOFF", v: 1 }, { t: "RTS/CTS", v: 2 } ]
                        currentIndex: 0
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                }
                RowLayout {
                    Label { text: "编码"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    ComboBox {
                        id: encCombo
                        Layout.fillWidth: true
                        textRole: "label"; valueRole: "cp"
                        model: ZSApplication.availableEncodings()
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                }
                RowLayout {
                    Label { text: "分组"; color: "#6b7280"; Layout.preferredWidth: 70 }
                    TextField {
                        id: groupField
                        Layout.fillWidth: true
                        placeholderText: "例如：开发板（留空为默认分组）"
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                }

                Item { Layout.fillHeight: true }

                Label {
                    text: "配置文件：" + ZSApplication.sessionStorePath()
                    color: "#7b828c"; font.pointSize: 9
                    Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
                }
            }

            // ---------- 右：终端选项 + 日志 ----------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                Label { text: "终端选项"; color: "#2f6fed"; font.bold: true; font.pointSize: 12 }

                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    rowSpacing: 8; columnSpacing: 8
                    CheckBox { id: tsCheck; text: "显示时间戳"; checked: false; Layout.fillWidth: true }
                    CheckBox { id: hexCheck; text: "HEX 显示"; checked: false; Layout.fillWidth: true }
                    CheckBox { id: echoCheck; text: "本地回显"; checked: false; Layout.fillWidth: true }
                    CheckBox { id: scrollCheck; text: "自动滚动"; checked: true; Layout.fillWidth: true }
                    CheckBox { id: reconnectCheck; text: "自动重连"; checked: false; Layout.fillWidth: true }
                    CheckBox { id: crlfCheck; text: "CR+LF 换行"; checked: false; Layout.fillWidth: true }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d0d7e0" }

                Label { text: "日志记录"; color: "#2f6fed"; font.bold: true; font.pointSize: 12 }

                RowLayout {
                    Layout.fillWidth: true
                    CheckBox { id: logCheck; text: "保存到日志文件"; checked: false }
                    Item { Layout.fillWidth: true }
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: logPathField
                        Layout.fillWidth: true
                        placeholderText: "日志目录"
                        enabled: logCheck.checked
                        background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                    }
                    Button {
                        text: "浏览"
                        enabled: logCheck.checked
                        implicitWidth: 56
                        onClicked: folderDlg.open()
                        background: Rectangle { color: parent.hovered ? "#d3dbe6" : "#e3e8ee"; border.color: "#d0d7e0"; radius: 4 }
                        contentItem: Label { text: parent.text; color: "#2b2f36"; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Label {
            id: hint
            Layout.fillWidth: true
            text: ""
            color: "#b7791f"
            font.pointSize: 9
            wrapMode: Text.Wrap
        }

        Label {
            Layout.fillWidth: true
            text: dlg.tipText
            color: dlg.tipOk ? "#2e7d32" : "#c0392b"
            font.pointSize: 9
            wrapMode: Text.Wrap
            visible: dlg.tipText !== ""
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "测试连接"
                implicitWidth: 92
                onClicked: dlg.doTest()
                background: Rectangle { color: parent.hovered ? "#d3dbe6" : "#e3e8ee"; border.color: "#d0d7e0"; radius: 4 }
                contentItem: Label { text: parent.text; color: "#2b2f36"; horizontalAlignment: Text.AlignHCenter }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "取消"
                implicitWidth: 80
                onClicked: dlg.close()
                background: Rectangle { color: parent.hovered ? "#d3dbe6" : "#e3e8ee"; border.color: "#d0d7e0"; radius: 4 }
                contentItem: Label { text: parent.text; color: "#2b2f36"; horizontalAlignment: Text.AlignHCenter }
            }
            Button {
                text: "保存并连接"
                implicitWidth: 112
                onClicked: dlg.doSave(true)
                background: Rectangle { color: parent.hovered ? "#2459c4" : "#2f6fed"; radius: 4 }
                contentItem: Label { text: parent.text; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

Window {
    id: dlg
    title: "新建串口会话"
    width: 470
    height: 520
    minimumWidth: 420
    minimumHeight: 460
    modality: Qt.ApplicationModal
    color: "#eef1f5"

    signal saved(string name)

    function exists(n) {
        var names = ZSApplication.sessionNames();
        for (var i = 0; i < names.length; ++i)
            if (names[i] === n) return true;
        return false;
    }

    // 用主窗口当前参数回填，避免用户重复设置。
    function openWith(port, baud, data, stop, parity, flow, cp) {
        var ports = ZSApplication.availablePorts();
        nameField.text = "";
        hint.text = "";
        portCombo.model = ports;
        if (port && port !== "" && portCombo.find(port) >= 0)
            portCombo.currentIndex = portCombo.find(port);
        else
            portCombo.editText = (port && port !== "") ? port : (ports.length ? ports[0] : "COM1");
        baudCombo.currentIndex = Math.max(0, baudCombo.indexOfValue(baud));
        dataCombo.currentIndex = Math.max(0, dataCombo.indexOfValue(data));
        stopCombo.currentIndex = Math.max(0, stopCombo.indexOfValue(stop));
        parityCombo.currentIndex = Math.max(0, parityCombo.indexOfValue(parity));
        flowCombo.currentIndex = Math.max(0, flowCombo.indexOfValue(flow));
        encCombo.currentIndex = Math.max(0, encCombo.indexOfValue(
            (cp && cp !== "") ? cp : ZSApplication.defaultEncoding()));
        dlg.show();
        nameField.forceActiveFocus();
    }

    function doSave() {
        var n = nameField.text.trim();
        if (n === "") {
            hint.text = "请输入会话名";
            hint.color = "#c0392b";
            return;
        }
        ZSApplication.saveSession(n, portCombo.displayText,
                                  baudCombo.currentValue,
                                  dataCombo.currentValue,
                                  stopCombo.currentValue,
                                  parityCombo.currentValue,
                                  flowCombo.currentValue,
                                  encCombo.currentValue);
        saved(n);
        dlg.close();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "新建串口会话"
            color: "#2b2f36"
            font.bold: true
            font.pointSize: 14
            Layout.bottomMargin: 2
        }

        RowLayout {
            Label { text: "会话名"; color: "#6b7280"; Layout.preferredWidth: 76 }
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "例如：COM4-调试"
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
                onTextChanged: {
                    var trimmed = text.trim();
                    hint.color = "#b7791f";
                    hint.text = (dlg.exists(trimmed) && trimmed !== "")
                                ? "同名会话已存在，保存将覆盖" : "";
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d0d7e0" }
        Label { text: "串口参数"; color: "#2b2f36"; font.bold: true; font.pointSize: 12 }

        RowLayout {
            Label { text: "端口"; color: "#6b7280"; Layout.preferredWidth: 76 }
            ComboBox {
                id: portCombo
                editable: true
                Layout.fillWidth: true
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
            }
        }
        RowLayout {
            Label { text: "波特率"; color: "#6b7280"; Layout.preferredWidth: 76 }
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
            Label { text: "数据位"; color: "#6b7280"; Layout.preferredWidth: 76 }
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
                Layout.preferredWidth: 90
                valueRole: "v"; textRole: "t"
                model: [ { t: "1", v: 1 }, { t: "1.5", v: 3 }, { t: "2", v: 2 } ]
                currentIndex: 0
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
            }
        }
        RowLayout {
            Label { text: "校验位"; color: "#6b7280"; Layout.preferredWidth: 76 }
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
                Layout.preferredWidth: 100
                valueRole: "v"; textRole: "t"
                model: [ { t: "无", v: 0 }, { t: "XON/XOFF", v: 1 }, { t: "RTS/CTS", v: 2 } ]
                currentIndex: 0
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
            }
        }
        RowLayout {
            Label { text: "编码"; color: "#6b7280"; Layout.preferredWidth: 76 }
            ComboBox {
                id: encCombo
                Layout.fillWidth: true
                textRole: "label"; valueRole: "cp"
                model: ZSApplication.availableEncodings()
                background: Rectangle { color: "#ffffff"; border.color: "#d0d7e0"; radius: 4 }
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

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#d0d7e0" }

        Label {
            text: "保存位置：" + ZSApplication.sessionStorePath()
            color: "#7b828c"
            font.pointSize: 9
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                id: cancelBtn
                text: "取消"
                implicitWidth: 84
                onClicked: dlg.close()
                background: Rectangle {
                    color: cancelBtn.hovered ? "#d3dbe6" : "#e3e8ee"
                    border.color: "#d0d7e0"; radius: 4
                }
                contentItem: Label {
                    text: cancelBtn.text; color: "#2b2f36"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
            Button {
                id: saveBtn
                text: "保存会话"
                implicitWidth: 96
                onClicked: dlg.doSave()
                background: Rectangle {
                    color: saveBtn.hovered ? "#1f4f86" : "#2b6cb0"; radius: 4
                }
                contentItem: Label {
                    text: saveBtn.text; color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}

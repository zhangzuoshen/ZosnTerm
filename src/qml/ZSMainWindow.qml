import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

Window {
    id: root
    width: 900
    height: 640
    minimumWidth: 480
    minimumHeight: 360
    title: "ZosnTerm"
    visible: true
    color: "#1c1e22"

    // 把一组串口参数回填到各 ComboBox。
    function applyParams(port, baud, data, stop, parity, flow) {
        if (portCombo.find(port) >= 0) portCombo.currentIndex = portCombo.find(port);
        else portCombo.editText = port;
        baudCombo.currentIndex = baudCombo.indexOfValue(baud);
        dataCombo.currentIndex = dataCombo.indexOfValue(data);
        stopCombo.currentIndex = stopCombo.indexOfValue(stop);
        parityCombo.currentIndex = parityCombo.indexOfValue(parity);
        flowCombo.currentIndex = flowCombo.indexOfValue(flow);
    }

    Component.onCompleted: {
        portCombo.model = ZSApplication.availablePorts();
        sessionCombo.model = ZSApplication.sessionNames();
        var last = ZSApplication.lastParams();
        if (last.port !== undefined && last.port !== "")
            applyParams(last.port, last.baudRate, last.dataBits,
                        last.stopBits, last.parity, last.flow);
    }

    Connections {
        target: ZSApplication
        function onConnectionParamsApplied(port, baud, data, stop, parity, flow) {
            applyParams(port, baud, data, stop, parity, flow);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            background: Rectangle { color: "#26282d" }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                ComboBox {
                    id: portCombo
                    editable: true
                    implicitWidth: 130
                }
                ComboBox {
                    id: baudCombo
                    implicitWidth: 90
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
                    implicitWidth: 64
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
                    implicitWidth: 64
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
                    implicitWidth: 72
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
                    implicitWidth: 90
                    valueRole: "v"
                    textRole: "t"
                    model: [
                        { t: "无",       v: 0 },
                        { t: "XON/XOFF", v: 1 },
                        { t: "RTS/CTS",  v: 2 }
                    ]
                    currentIndex: 0
                }
                ToolButton {
                    text: "连接"
                    onClicked: ZSApplication.connectSerial(
                        portCombo.displayText,
                        baudCombo.currentValue,
                        dataCombo.currentValue,
                        stopCombo.currentValue,
                        parityCombo.currentValue,
                        flowCombo.currentValue)
                }
                ToolButton {
                    text: "断开"
                    onClicked: ZSApplication.closeSerialSession()
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: "M4 会话管理 · PuTTY 0.85 终端仿真"
                    color: "#8a8f98"
                    font.pointSize: 10
                }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            background: Rectangle { color: "#202227" }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                Label { text: "会话"; color: "#8a8f98" }
                ComboBox {
                    id: sessionCombo
                    implicitWidth: 150
                    model: []
                }
                TextField {
                    id: sessionName
                    implicitWidth: 120
                    placeholderText: "会话名"
                }
                ToolButton {
                    text: "保存"
                    onClicked: {
                        var nm = sessionName.text.trim();
                        if (nm !== "") {
                            ZSApplication.saveSession(nm,
                                portCombo.displayText,
                                baudCombo.currentValue,
                                dataCombo.currentValue,
                                stopCombo.currentValue,
                                parityCombo.currentValue,
                                flowCombo.currentValue);
                            sessionCombo.model = ZSApplication.sessionNames();
                            sessionCombo.currentIndex = sessionCombo.find(nm);
                        }
                    }
                }
                ToolButton {
                    text: "加载"
                    onClicked: {
                        if (sessionCombo.currentIndex >= 0)
                            ZSApplication.loadSession(sessionCombo.currentText);
                    }
                }
                ToolButton {
                    text: "删除"
                    onClicked: {
                        if (sessionCombo.currentIndex >= 0) {
                            ZSApplication.deleteSession(sessionCombo.currentText);
                            sessionCombo.model = ZSApplication.sessionNames();
                            sessionCombo.currentIndex = -1;
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        ZSTerminalView {
            id: term
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#26282d"
            Label {
                anchors.fill: parent
                leftPadding: 8
                verticalAlignment: Qt.AlignVCenter
                text: ZSApplication.statusMessage
                color: "#c8ccd2"
                font.pointSize: 10
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZosnTerm 1.0

Window {
    id: root
    width: 900
    height: 600
    minimumWidth: 480
    minimumHeight: 320
    title: "ZosnTerm"
    visible: true
    color: "#1c1e22"

    Component.onCompleted: {
        portCombo.model = ZSApplication.availablePorts()
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
                    text: "M3 串口参数可配置 · PuTTY 0.85 终端仿真"
                    color: "#8a8f98"
                    font.pointSize: 10
                }
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

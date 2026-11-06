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

                ToolButton {
                    text: "新建串口"
                    onClicked: ZSApplication.newSerialSession()
                }
                ToolButton {
                    text: "断开"
                    onClicked: ZSApplication.closeSerialSession()
                }
                ToolButton {
                    text: "会话管理"
                    enabled: false   // M4 接入
                }
                ToolButton {
                    text: "设置"
                    enabled: false   // M5 接入
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: "M2 引擎已接入 · PuTTY 0.85 终端仿真"
                    color: "#8a8f98"
                    font.pointSize: 10
                }
            }
        }

        ZSTerminalView {
            id: term
            Layout.fillWidth: true
            Layout.fillHeight: true
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

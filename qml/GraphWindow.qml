import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CodeReader 1.0

Window {
    id: graphWin
    width: 1100
    height: 750
    minimumWidth: 800
    minimumHeight: 600
    title: isFileGraph ? "全项目文件依赖网络拓扑图 (Graphviz & QPainter)" : "文件内部符号调用结构图 (Graphviz & QPainter)"
    color: themeBackground

    property string dotSource: ""
    property bool isFileGraph: true
    property color themeBackground: "#0f172a"
    property color themeCard: "#1e293b"
    property color themeBorder: "#334155"
    property color themeText: "#f8fafc"

    signal nodeSelected(string nodeId, string fullPath)
    signal nodeDoubleClicked(string nodeId, string fullPath)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top Toolbar
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: themeCard
            border.color: themeBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: isFileGraph ? "🌐 全项目文件依赖网络" : "📦 文件内部符号结构"
                    color: themeText
                    font.bold: true
                    font.pixelSize: 13
                }

                Rectangle { width: 1; height: 20; color: themeBorder }

                Button {
                    text: winGraphView.layoutDirection === "LR" ? "布局: 水平 (LR)" : "布局: 垂直 (TB)"
                    background: Rectangle {
                        color: parent.down ? "#334155" : "#0f172a"
                        border.color: themeBorder
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: "#cbd5e1"; font.pixelSize: 11 }
                    onClicked: winGraphView.layoutDirection = (winGraphView.layoutDirection === "LR") ? "TB" : "LR"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: winGraphView.statusMessage
                    color: "#94a3b8"
                    font.pixelSize: 11
                }

                Button { text: "➕"; implicitWidth: 32; implicitHeight: 30; onClicked: winGraphView.zoomIn() }
                Button { text: "➖"; implicitWidth: 32; implicitHeight: 30; onClicked: winGraphView.zoomOut() }
                Button { text: "适应窗口"; implicitHeight: 30; onClicked: winGraphView.fitToView() }
                Button { text: "复位视角"; implicitHeight: 30; onClicked: winGraphView.resetView() }

                Button {
                    text: "关闭窗口"
                    implicitHeight: 30
                    background: Rectangle {
                        color: "#ef4444"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 11
                    }
                    onClicked: graphWin.close()
                }
            }
        }

        // Graph Canvas
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            InteractiveGraphView {
                id: winGraphView
                anchors.fill: parent
                dotSource: graphWin.dotSource

                onNodeDoubleClicked: (nodeId, fullPath) => {
                    graphWin.nodeDoubleClicked(nodeId, fullPath)
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.MiddleButton

                    property real lastX: 0
                    property real lastY: 0

                    onPressed: (mouse) => {
                        lastX = mouse.x
                        lastY = mouse.y
                        winGraphView.handleMousePress(mouse.x, mouse.y)
                    }

                    onPositionChanged: (mouse) => {
                        winGraphView.handleMouseMove(mouse.x, mouse.y)
                        if (mouse.buttons & Qt.LeftButton || mouse.buttons & Qt.MiddleButton) {
                            var dx = mouse.x - lastX
                            var dy = mouse.y - lastY
                            winGraphView.panBy(dx, dy)
                            lastX = mouse.x
                            lastY = mouse.y
                        }
                    }

                    onWheel: (wheel) => {
                        winGraphView.handleWheel(wheel.x, wheel.y, wheel.angleDelta.y)
                    }
                }
            }
        }
    }
}

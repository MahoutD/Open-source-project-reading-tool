import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import CodeReader 1.0

ApplicationWindow {
    id: window
    width: 1380
    height: 860
    minimumWidth: 1024
    minimumHeight: 680
    visible: true
    title: "开源代码全景阅读器 (Open-Source Project Reader)"
    color: "#18181b"

    property string currentSelectedFilePath: ""
    property string currentFileExtension: "cpp"

    // Top Navigation & Actions Bar
    header: ToolBar {
        background: Rectangle {
            color: "#1f1f23"
            border.color: "#2e2e33"
            border.width: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 12

            Text {
                text: "🔍 CodeReader"
                color: "#60a5fa"
                font.bold: true
                font.pixelSize: 16
            }

            Rectangle { width: 1; height: 24; color: "#3f3f46" }

            // URL input & Clone
            TextField {
                id: repoInput
                Layout.fillWidth: true
                placeholderText: "输入 GitHub / Gitee / GitLab 仓库链接 (例如: https://github.com/nlohmann/json.git)"
                color: "#f4f4f5"
                placeholderTextColor: "#71717a"
                background: Rectangle {
                    color: "#27272a"
                    radius: 6
                    border.color: repoInput.activeFocus ? "#3b82f6" : "#3f3f46"
                }
            }

            Button {
                text: projectController.isBusy ? "拉取中..." : "拉取并阅读"
                enabled: !projectController.isBusy && repoInput.text.trim().length > 0
                background: Rectangle {
                    color: parent.enabled ? (parent.down ? "#1d4ed8" : "#2563eb") : "#3f3f46"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: projectController.cloneAndOpenRepo(repoInput.text)
            }

            Button {
                text: "打开本地文件夹"
                background: Rectangle {
                    color: parent.down ? "#27272a" : "#3f3f46"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#e4e4e7"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: folderDialog.open()
            }

            Rectangle { width: 1; height: 24; color: "#3f3f46" }

            Button {
                text: "🌐 连通性测试"
                background: Rectangle {
                    color: parent.down ? "#047857" : "#059669"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: netDialog.open()
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "选择项目源代码根目录"
        currentFolder: "file:///" + projectController.currentProjectPath
        onAccepted: {
            projectController.openLocalFolder(selectedFolder.toString())
        }
    }

    // Main workspace split
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Left Sidebar: File Tree & Project Summary Switch
        Rectangle {
            SplitView.preferredWidth: 320
            SplitView.minimumWidth: 240
            SplitView.maximumWidth: 500
            color: "#18181b"
            border.color: "#27272a"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Sidebar tabs
                TabBar {
                    id: sideTab
                    Layout.fillWidth: true
                    background: Rectangle { color: "#1f1f23" }

                    TabButton {
                        text: "📁 文件关系列表"
                        contentItem: Text {
                            text: parent.text
                            color: parent.checked ? "#60a5fa" : "#a1a1aa"
                            font.bold: parent.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.checked ? "#27272a" : "transparent"
                        }
                    }

                    TabButton {
                        text: "📋 全景总结"
                        contentItem: Text {
                            text: parent.text
                            color: parent.checked ? "#60a5fa" : "#a1a1aa"
                            font.bold: parent.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.checked ? "#27272a" : "transparent"
                        }
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: sideTab.currentIndex

                    // Item 0: File List
                    Rectangle {
                        color: "#18181b"

                        ListView {
                            id: fileListView
                            anchors.fill: parent
                            clip: true
                            model: projectController.fileList

                            delegate: Rectangle {
                                width: fileListView.width
                                height: 42
                                color: currentSelectedFilePath === modelData.path ? "#27272a" : (maItem.containsMouse ? "#202023" : "transparent")
                                border.color: currentSelectedFilePath === modelData.path ? "#3b82f6" : "transparent"
                                border.width: 1

                                MouseArea {
                                    id: maItem
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        currentSelectedFilePath = modelData.path
                                        currentFileExtension = modelData.type
                                        codeEditor.text = projectController.readFileContent(modelData.path)
                                        editorBridge.fileExtension = modelData.type
                                        projectController.selectFile(modelData.path)
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    Text {
                                        text: {
                                            var ext = modelData.type
                                            if (ext === "h" || ext === "hpp") return "📄 [H]"
                                            if (ext === "cpp" || ext === "c") return "⚙️ [C++]"
                                            if (ext === "qml") return "🎨 [QML]"
                                            if (ext === "py") return "🐍 [Py]"
                                            return "📝 [Txt]"
                                        }
                                        color: "#a1a1aa"
                                        font.pixelSize: 11
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            text: modelData.name
                                            color: "#f4f4f5"
                                            font.bold: true
                                            font.pixelSize: 13
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: modelData.path + "  (" + modelData.lines + "行, " + modelData.dependenciesCount + "个依赖)"
                                            color: "#71717a"
                                            font.pixelSize: 10
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            ScrollBar.vertical: ScrollBar { active: true }
                        }
                    }

                    // Item 1: Project Summary
                    ScrollView {
                        clip: true
                        TextArea {
                            readOnly: true
                            text: projectController.projectSummary
                            textFormat: TextEdit.MarkdownText
                            color: "#e4e4e7"
                            font.pixelSize: 13
                            background: Rectangle { color: "#18181b" }
                            padding: 16
                        }
                    }
                }
            }
        }

        // Center / Right: Code Viewer & Graph Visualization
        SplitView {
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            // Upper View: Code Editor (Scintilla-style with Line Numbers & Highlighting)
            Rectangle {
                SplitView.preferredHeight: 460
                SplitView.fillWidth: true
                color: "#1e1e1e"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // File Header Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#252526"
                        border.color: "#333333"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                text: currentSelectedFilePath.length > 0 ? ("📝 " + currentSelectedFilePath) : "未选择文件"
                                color: "#cccccc"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "语法高亮引擎就绪 | UTF-8"
                                color: "#858585"
                                font.pixelSize: 11
                            }
                        }
                    }

                    // Editor Workspace
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        // Line Number Gutter
                        Rectangle {
                            Layout.fillHeight: true
                            width: 50
                            color: "#1e1e1e"
                            border.color: "#2d2d2d"

                            ListView {
                                id: lineNumList
                                anchors.fill: parent
                                clip: true
                                interactive: false
                                contentY: codeScrollView.contentItem.contentY
                                model: Math.max(1, codeEditor.lineCount)

                                delegate: Item {
                                    width: lineNumList.width
                                    height: codeEditor.cursorRectangle.height > 0 ? codeEditor.cursorRectangle.height : 18
                                    Text {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: index + 1
                                        color: "#858585"
                                        font.family: "Consolas, 'Courier New', monospace"
                                        font.pixelSize: 13
                                    }
                                }
                            }
                        }

                        // Code Editor Body
                        ScrollView {
                            id: codeScrollView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: codeEditor
                                font.family: "Consolas, 'Cascadia Code', 'Courier New', monospace"
                                font.pixelSize: 13
                                color: "#d4d4d4"
                                selectionColor: "#264f78"
                                selectedTextColor: "#ffffff"
                                selectByMouse: true
                                wrapMode: TextArea.NoWrap
                                background: Rectangle { color: "#1e1e1e" }
                                leftPadding: 8
                                topPadding: 4

                                CodeEditorBridge {
                                    id: editorBridge
                                    textDocument: codeEditor.textDocument
                                    fileExtension: currentFileExtension
                                }
                            }
                        }
                    }
                }
            }

            // Lower View: Graphviz & Internal Dependency Visualizer
            Rectangle {
                SplitView.fillHeight: true
                SplitView.fillWidth: true
                color: "#18181b"
                border.color: "#27272a"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Graph Control Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        color: "#202023"
                        border.color: "#27272a"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12

                            Text {
                                text: "📊 依赖拓扑图 (Graphviz & QGraphicsView Engine)"
                                color: "#e4e4e7"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Row {
                                spacing: 4
                                Button {
                                    text: "全项目依赖图"
                                    checked: true
                                    onClicked: {
                                        graphView.dotSource = projectController.fileGraphDot
                                    }
                                }
                                Button {
                                    text: "当前文件内部关系图"
                                    onClicked: {
                                        graphView.dotSource = projectController.currentSymbolGraphDot
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: graphView.statusMessage
                                color: "#a1a1aa"
                                font.pixelSize: 11
                            }

                            Button { text: "+"; width: 32; onClicked: graphView.zoomIn() }
                            Button { text: "-"; width: 32; onClicked: graphView.zoomOut() }
                            Button { text: "重置缩放"; onClicked: graphView.resetView() }
                        }
                    }

                    // Interactive Graphviz Scene Canvas
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        GraphVisualizer {
                            id: graphView
                            anchors.fill: parent
                            dotSource: projectController.fileGraphDot

                            Connections {
                                target: projectController
                                function onFileGraphDotChanged() {
                                    graphView.dotSource = projectController.fileGraphDot
                                }
                                function onCurrentSymbolGraphDotChanged() {
                                    graphView.dotSource = projectController.currentSymbolGraphDot
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                                drag.target: null

                                property real lastX: 0
                                property real lastY: 0

                                onPressed: (mouse) => {
                                    lastX = mouse.x
                                    lastY = mouse.y
                                }

                                onPositionChanged: (mouse) => {
                                    if (mouse.buttons & Qt.LeftButton || mouse.buttons & Qt.MiddleButton) {
                                        var dx = mouse.x - lastX
                                        var dy = mouse.y - lastY
                                        graphView.panBy(dx, dy)
                                        lastX = mouse.x
                                        lastY = mouse.y
                                    }
                                }

                                onWheel: (wheel) => {
                                    if (wheel.angleDelta.y > 0) {
                                        graphView.zoomIn()
                                    } else {
                                        graphView.zoomOut()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Bottom Status Bar
    footer: Rectangle {
        height: 28
        color: "#007acc"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            Text {
                text: "状态: " + projectController.statusMessage
                color: "#ffffff"
                font.pixelSize: 11
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "MSVC x64 + Qt 6.8.3 QML + Graphviz 核心驱动"
                color: "#e0e0e0"
                font.pixelSize: 11
            }
        }
    }

    // Network Connectivity Dialog
    Dialog {
        id: netDialog
        title: "代码托管平台网络连通性测试"
        modal: true
        anchors.centerIn: parent
        width: 620
        height: 480
        background: Rectangle {
            color: "#1f1f23"
            radius: 8
            border.color: "#3f3f46"
        }

        header: Rectangle {
            height: 48
            color: "#27272a"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Text {
                    text: "🌐 代码托管平台网络连通性测试"
                    color: "#f4f4f5"
                    font.bold: true
                    font.pixelSize: 14
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: networkTester.testing ? "测试中..." : "重新测试全部"
                    enabled: !networkTester.testing
                    onClicked: networkTester.testAllPlatforms()
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: 12

            ListModel {
                id: netResultsModel
                ListElement { platform: "GitHub"; key: "github"; url: "https://github.com"; status: "未检测"; latency: "-"; ok: false }
                ListElement { platform: "Gitee (码云)"; key: "gitee"; url: "https://gitee.com"; status: "未检测"; latency: "-"; ok: false }
                ListElement { platform: "GitLab"; key: "gitlab"; url: "https://gitlab.com"; status: "未检测"; latency: "-"; ok: false }
                ListElement { platform: "GitCode"; key: "gitcode"; url: "https://gitcode.com"; status: "未检测"; latency: "-"; ok: false }
            }

            Connections {
                target: networkTester
                function onTestResultReady(key, success, code, latencyMs, message) {
                    for (var i = 0; i < netResultsModel.count; ++i) {
                        var item = netResultsModel.get(i)
                        if (item.key === key) {
                            item.status = message
                            item.latency = latencyMs + " ms"
                            item.ok = success
                            break
                        }
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: netResultsModel
                spacing: 8

                delegate: Rectangle {
                    width: parent.width
                    height: 64
                    color: "#27272a"
                    radius: 6
                    border.color: model.ok ? "#10b981" : "#ef4444"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Rectangle {
                            width: 12
                            height: 12
                            radius: 6
                            color: model.ok ? "#10b981" : (model.status === "未检测" ? "#71717a" : "#ef4444")
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Text {
                                    text: model.platform
                                    color: "#f4f4f5"
                                    font.bold: true
                                    font.pixelSize: 13
                                }
                                Text {
                                    text: "(" + model.url + ")"
                                    color: "#a1a1aa"
                                    font.pixelSize: 11
                                }
                            }
                            Text {
                                text: model.status
                                color: model.ok ? "#34d399" : (model.status === "未检测" ? "#71717a" : "#f87171")
                                font.pixelSize: 11
                            }
                        }

                        Text {
                            text: model.latency
                            color: "#60a5fa"
                            font.bold: true
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }

        onOpened: {
            networkTester.testAllPlatforms()
        }
    }
}

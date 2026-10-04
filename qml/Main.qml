import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import CodeReader 1.0

ApplicationWindow {
    id: window
    width: 1440
    height: 900
    minimumWidth: 1080
    minimumHeight: 700
    visible: true
    title: "CodeInsight Pro - 开源代码架构与依赖全景分析工具"
    color: "#0f172a" // Slate-900 ultra modern dark

    property string currentSelectedFilePath: ""
    property string currentFileExtension: "cpp"
    property bool isFileGraphActive: true

    // Top Command / Navigation Bar
    header: ToolBar {
        height: 54
        background: Rectangle {
            color: "#1e293b" // Slate-800
            border.color: "#334155"
            border.width: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Brand / Logo
            RowLayout {
                spacing: 8
                Rectangle {
                    width: 30
                    height: 30
                    radius: 6
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#38bdf8" }
                        GradientStop { position: 1.0; color: "#0284c7" }
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "⌘"
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 16
                    }
                }
                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "CodeInsight Pro"
                        color: "#f8fafc"
                        font.bold: true
                        font.pixelSize: 14
                    }
                    Text {
                        text: "开源代码全景阅读与拓扑分析系统"
                        color: "#94a3b8"
                        font.pixelSize: 10
                    }
                }
            }

            Rectangle { width: 1; height: 26; color: "#334155" }

            // URL Input Box with Repo Icon
            Rectangle {
                Layout.fillWidth: true
                Layout.maximumWidth: 540
                height: 36
                color: "#0f172a"
                radius: 6
                border.color: repoInput.activeFocus ? "#38bdf8" : "#334155"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 8
                    spacing: 6

                    Text {
                        text: "🔗"
                        color: "#64748b"
                        font.pixelSize: 12
                    }

                    TextField {
                        id: repoInput
                        Layout.fillWidth: true
                        placeholderText: "输入 GitHub / Gitee / GitLab 仓库链接 (支持 https://... 或 git@...)"
                        color: "#f8fafc"
                        placeholderTextColor: "#64748b"
                        font.pixelSize: 12
                        background: Item {}
                        selectByMouse: true
                    }

                    Button {
                        text: projectController.isBusy ? "拉取中..." : "拉取分析"
                        enabled: !projectController.isBusy && repoInput.text.trim().length > 0
                        implicitHeight: 28
                        background: Rectangle {
                            color: parent.enabled ? (parent.down ? "#0284c7" : "#0284c7") : "#334155"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.bold: true
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            appLogger.logInfo("用户操作", "点击拉取仓库: " + repoInput.text.trim());
                            projectController.cloneAndOpenRepo(repoInput.text);
                        }
                    }
                }
            }

            // Open Local Folder Button
            Button {
                text: "📂 打开本地代码"
                implicitHeight: 34
                background: Rectangle {
                    color: parent.down ? "#334155" : "#1e293b"
                    border.color: "#475569"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#f1f5f9"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: folderDialog.open()
            }

            // Quick Refresh Button
            Button {
                text: "🔄 重新分析"
                implicitHeight: 34
                enabled: !projectController.isBusy
                background: Rectangle {
                    color: parent.down ? "#334155" : "#1e293b"
                    border.color: "#475569"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#f1f5f9"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    appLogger.logInfo("用户操作", "触发重新分析当前工程");
                    projectController.refreshAnalysis();
                }
            }

            Item { Layout.fillWidth: true }

            // Action: Network Test
            Button {
                text: "🌐 网络连通性诊断"
                implicitHeight: 34
                background: Rectangle {
                    color: parent.down ? "#047857" : "#059669"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    appLogger.logInfo("用户操作", "打开网络连通性诊断对话框");
                    netDialog.open();
                }
            }

            // Action: Help & About
            Button {
                text: "❓ 使用帮助"
                implicitHeight: 34
                background: Rectangle {
                    color: parent.down ? "#334155" : "#1e293b"
                    border.color: "#475569"
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#cbd5e1"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: helpDialog.open()
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "选择项目源码根目录"
        currentFolder: "file:///" + projectController.currentProjectPath
        onAccepted: {
            appLogger.logInfo("用户操作", "选择本地工程目录: " + selectedFolder.toString());
            projectController.openLocalFolder(selectedFolder.toString());
        }
    }

    // Main 3-Area Split Layout
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // ==========================================
        // LEFT PANE: Project Explorer & Summary
        // ==========================================
        Rectangle {
            SplitView.preferredWidth: 340
            SplitView.minimumWidth: 260
            SplitView.maximumWidth: 520
            color: "#0f172a"
            border.color: "#1e293b"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header with custom styled tabs
                Rectangle {
                    Layout.fillWidth: true
                    height: 44
                    color: "#1e293b"
                    border.color: "#334155"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        Button {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: "📁 源码文件 (" + projectController.fileList.length + ")"
                            background: Rectangle {
                                color: sideStack.currentIndex === 0 ? "#0f172a" : "transparent"
                                radius: 4
                                border.color: sideStack.currentIndex === 0 ? "#38bdf8" : "transparent"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                color: sideStack.currentIndex === 0 ? "#38bdf8" : "#94a3b8"
                                font.bold: sideStack.currentIndex === 0
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: sideStack.currentIndex = 0
                        }

                        Button {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: "📋 全景总结报告"
                            background: Rectangle {
                                color: sideStack.currentIndex === 1 ? "#0f172a" : "transparent"
                                radius: 4
                                border.color: sideStack.currentIndex === 1 ? "#38bdf8" : "transparent"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                color: sideStack.currentIndex === 1 ? "#38bdf8" : "#94a3b8"
                                font.bold: sideStack.currentIndex === 1
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: sideStack.currentIndex = 1
                        }
                    }
                }

                // Search / Filter Input for Files
                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    color: "#131b2e"
                    visible: sideStack.currentIndex === 0
                    border.color: "#1e293b"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Text { text: "🔍"; color: "#64748b"; font.pixelSize: 11 }

                        TextField {
                            id: fileFilterInput
                            Layout.fillWidth: true
                            placeholderText: "快速过滤文件名..."
                            color: "#f8fafc"
                            placeholderTextColor: "#64748b"
                            font.pixelSize: 11
                            background: Item {}
                        }

                        Button {
                            text: "×"
                            visible: fileFilterInput.text.length > 0
                            implicitWidth: 20
                            implicitHeight: 20
                            background: Item {}
                            contentItem: Text { text: "×"; color: "#94a3b8"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                            onClicked: fileFilterInput.text = ""
                        }
                    }
                }

                // Stack Container
                StackLayout {
                    id: sideStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: 0

                    // View 0: Source File List
                    ListView {
                        id: fileListView
                        clip: true
                        model: {
                            var q = fileFilterInput.text.toLowerCase().trim()
                            if (q.length === 0) return projectController.fileList
                            var filtered = []
                            for (var i = 0; i < projectController.fileList.length; ++i) {
                                var item = projectController.fileList[i]
                                if (item.name.toLowerCase().indexOf(q) !== -1 || item.path.toLowerCase().indexOf(q) !== -1) {
                                    filtered.push(item)
                                }
                            }
                            return filtered
                        }

                        delegate: Rectangle {
                            width: fileListView.width
                            height: 48
                            color: currentSelectedFilePath === modelData.path ? "#1e293b" : (maItem.containsMouse ? "#172033" : "transparent")
                            border.color: currentSelectedFilePath === modelData.path ? "#38bdf8" : "transparent"
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
                                    topoGraphView.selectNode(modelData.path)
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 10
                                spacing: 10

                                // Language tag
                                Rectangle {
                                    width: 32
                                    height: 22
                                    radius: 4
                                    color: {
                                        var ext = modelData.type
                                        if (ext === "h" || ext === "hpp") return "#0369a1"
                                        if (ext === "cpp" || ext === "c") return "#059669"
                                        if (ext === "qml") return "#7c3aed"
                                        if (ext === "py") return "#d97706"
                                        return "#475569"
                                    }
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.type.toUpperCase()
                                        color: "#ffffff"
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: modelData.name
                                        color: currentSelectedFilePath === modelData.path ? "#38bdf8" : "#f1f5f9"
                                        font.bold: currentSelectedFilePath === modelData.path
                                        font.pixelSize: 12
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.path + " • " + modelData.lines + " 行 • 依赖 " + modelData.dependenciesCount
                                        color: "#64748b"
                                        font.pixelSize: 10
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar { active: true }
                    }

                    // View 1: Summary Report with Export Button
                    ColumnLayout {
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 38
                            color: "#1e293b"
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                Text {
                                    text: "架构全景分析总结"
                                    color: "#f8fafc"
                                    font.bold: true
                                    font.pixelSize: 12
                                }
                                Item { Layout.fillWidth: true }
                                Button {
                                    text: "💾 导出报告"
                                    implicitHeight: 26
                                    background: Rectangle {
                                        color: "#0284c7"
                                        radius: 4
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#ffffff"
                                        font.bold: true
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        var exportFile = projectController.currentProjectPath + "/PROJECT_ANALYSIS_REPORT.md"
                                        if (projectController.exportReport(exportFile)) {
                                            exportNotice.open()
                                        }
                                    }
                                }
                            }
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                readOnly: true
                                text: projectController.projectSummary
                                textFormat: TextEdit.MarkdownText
                                color: "#e2e8f0"
                                font.pixelSize: 12
                                background: Rectangle { color: "#0f172a" }
                                padding: 14
                                wrapMode: TextArea.Wrap
                            }
                        }
                    }
                }
            }
        }

        // ==========================================
        // CENTER & RIGHT: Code Viewer + Topology Visualizer + Logs
        // ==========================================
        SplitView {
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            // TOP SECTION: Code Editor (Scintilla-like with gutter & syntax highlight)
            Rectangle {
                SplitView.preferredHeight: 460
                SplitView.minimumHeight: 200
                SplitView.fillWidth: true
                color: "#1e1e24" // VS Dark theme background

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Editor Title Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        color: "#25252b"
                        border.color: "#33333d"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: currentSelectedFilePath.length > 0 ? ("📄 " + currentSelectedFilePath) : "未选择任何文件"
                                color: "#f8fafc"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Rectangle {
                                width: 50
                                height: 20
                                radius: 3
                                color: "#334155"
                                visible: currentSelectedFilePath.length > 0
                                Text {
                                    anchors.centerIn: parent
                                    text: currentFileExtension.toUpperCase()
                                    color: "#38bdf8"
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "在拓扑图中聚焦该文件"
                                visible: currentSelectedFilePath.length > 0
                                implicitHeight: 26
                                background: Rectangle {
                                    color: "#334155"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#f8fafc"
                                    font.pixelSize: 11
                                }
                                onClicked: {
                                    projectController.selectFile(currentSelectedFilePath);
                                    topoGraphView.selectNode(currentSelectedFilePath);
                                }
                            }

                            Text {
                                text: "QSyntaxHighlighter 引擎驱动 | UTF-8"
                                color: "#64748b"
                                font.pixelSize: 10
                            }
                        }
                    }

                    // Editor Canvas with Line Numbers Gutter
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        // Line Numbers Bar
                        Rectangle {
                            Layout.fillHeight: true
                            width: 52
                            color: "#18181b"
                            border.color: "#27272a"

                            ListView {
                                id: lineNumList
                                anchors.fill: parent
                                clip: true
                                interactive: false
                                contentY: codeScrollView.contentItem.contentY
                                model: Math.max(1, codeEditor.lineCount)

                                delegate: Item {
                                    width: lineNumList.width
                                    height: codeEditor.cursorRectangle.height > 0 ? codeEditor.cursorRectangle.height : 19
                                    Text {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: index + 1
                                        color: "#52525b"
                                        font.family: "Consolas, 'Cascadia Code', monospace"
                                        font.pixelSize: 12
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
                                background: Rectangle { color: "#1e1e24" }
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

            // MIDDLE SECTION: Interactive Graphviz Scene & Topology View
            Rectangle {
                SplitView.fillHeight: true
                SplitView.preferredHeight: 380
                SplitView.minimumHeight: 180
                SplitView.fillWidth: true
                color: "#121216"
                border.color: "#1e293b"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Graph Toolbar & Status Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 42
                        color: "#1a1d24"
                        border.color: "#272c38"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                text: "📊 拓扑依赖图 (Graphviz & QPainter 矢量交互引擎)"
                                color: "#f8fafc"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Rectangle { width: 1; height: 18; color: "#334155" }

                            Button {
                                text: "全项目文件依赖网络"
                                implicitHeight: 28
                                background: Rectangle {
                                    color: isFileGraphActive ? "#0284c7" : "#1e293b"
                                    border.color: isFileGraphActive ? "#38bdf8" : "#475569"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#ffffff"
                                    font.bold: isFileGraphActive
                                    font.pixelSize: 11
                                }
                                onClicked: {
                                    isFileGraphActive = true;
                                    topoGraphView.dotSource = projectController.fileGraphDot;
                                    appLogger.logInfo("图谱切换", "切换显示: 全项目文件依赖网络");
                                }
                            }

                            Button {
                                text: "当前文件内部符号结构图"
                                implicitHeight: 28
                                background: Rectangle {
                                    color: !isFileGraphActive ? "#0284c7" : "#1e293b"
                                    border.color: !isFileGraphActive ? "#38bdf8" : "#475569"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#ffffff"
                                    font.bold: !isFileGraphActive
                                    font.pixelSize: 11
                                }
                                onClicked: {
                                    isFileGraphActive = false;
                                    topoGraphView.dotSource = projectController.currentSymbolGraphDot;
                                    appLogger.logInfo("图谱切换", "切换显示: 当前文件内部符号结构");
                                }
                            }

                            Button {
                                text: topoGraphView.layoutDirection === "LR" ? "布局: 水平 (LR)" : "布局: 垂直 (TB)"
                                implicitHeight: 28
                                background: Rectangle {
                                    color: "#1e293b"
                                    border.color: "#475569"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#cbd5e1"
                                    font.pixelSize: 11
                                }
                                onClicked: {
                                    topoGraphView.layoutDirection = (topoGraphView.layoutDirection === "LR") ? "TB" : "LR";
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: topoGraphView.statusMessage
                                color: "#94a3b8"
                                font.pixelSize: 11
                            }

                            Button { text: "➕"; implicitWidth: 32; implicitHeight: 28; onClicked: topoGraphView.zoomIn() }
                            Button { text: "➖"; implicitWidth: 32; implicitHeight: 28; onClicked: topoGraphView.zoomOut() }
                            Button { text: "适应窗口"; implicitHeight: 28; onClicked: topoGraphView.fitToView() }
                            Button { text: "复位"; implicitHeight: 28; onClicked: topoGraphView.resetView() }
                        }
                    }

                    // Interactive Graphviz Scene Container
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        InteractiveGraphView {
                            id: topoGraphView
                            anchors.fill: parent
                            dotSource: projectController.fileGraphDot

                            Connections {
                                target: projectController
                                function onFileGraphDotChanged() {
                                    if (isFileGraphActive) {
                                        topoGraphView.dotSource = projectController.fileGraphDot;
                                    }
                                }
                                function onCurrentSymbolGraphDotChanged() {
                                    if (!isFileGraphActive) {
                                        topoGraphView.dotSource = projectController.currentSymbolGraphDot;
                                    }
                                }
                            }

                            onNodeDoubleClicked: (nodeId, fullPath) => {
                                currentSelectedFilePath = fullPath;
                                currentFileExtension = fullPath.split('.').pop();
                                codeEditor.text = projectController.readFileContent(fullPath);
                                editorBridge.fileExtension = currentFileExtension;
                                appLogger.logInfo("图元联动", "双击图元并在编辑器中定位文件: " + fullPath);
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.LeftButton | Qt.MiddleButton

                                property real lastX: 0
                                property real lastY: 0
                                property bool isDragging: false

                                onPressed: (mouse) => {
                                    lastX = mouse.x
                                    lastY = mouse.y
                                    isDragging = false
                                    topoGraphView.handleMousePress(mouse.x, mouse.y)
                                }

                                onPositionChanged: (mouse) => {
                                    topoGraphView.handleMouseMove(mouse.x, mouse.y)
                                    if (mouse.buttons & Qt.LeftButton || mouse.buttons & Qt.MiddleButton) {
                                        var dx = mouse.x - lastX
                                        var dy = mouse.y - lastY
                                        if (Math.abs(dx) > 2 || Math.abs(dy) > 2) {
                                            isDragging = true
                                            topoGraphView.panBy(dx, dy)
                                        }
                                        lastX = mouse.x
                                        lastY = mouse.y
                                    }
                                }

                                onWheel: (wheel) => {
                                    topoGraphView.handleWheel(wheel.x, wheel.y, wheel.angleDelta.y)
                                }
                            }
                        }
                    }
                }
            }

            // BOTTOM SECTION: Operation & Diagnostics Log Terminal
            Rectangle {
                SplitView.preferredHeight: 140
                SplitView.minimumHeight: 90
                SplitView.maximumHeight: 280
                SplitView.fillWidth: true
                color: "#090d16"
                border.color: "#1e293b"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Log Header Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: "#131b2e"
                        border.color: "#1e293b"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: "📜 运行操作日志 (" + appLogger.count + " 条记录)"
                                color: "#f8fafc"
                                font.bold: true
                                font.pixelSize: 11
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "清空日志"
                                implicitHeight: 22
                                background: Rectangle {
                                    color: "#334155"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#cbd5e1"
                                    font.pixelSize: 10
                                }
                                onClicked: appLogger.clear()
                            }
                        }
                    }

                    // Log Entries List
                    ListView {
                        id: logListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: appLogger.logs

                        onCountChanged: {
                            logListView.positionViewAtEnd()
                        }

                        delegate: Rectangle {
                            width: logListView.width
                            height: 24
                            color: index % 2 === 0 ? "#090d16" : "#0d131f"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Text {
                                    text: modelData.timestamp
                                    color: "#64748b"
                                    font.family: "Consolas, monospace"
                                    font.pixelSize: 10
                                }

                                Rectangle {
                                    width: 54
                                    height: 18
                                    radius: 3
                                    color: {
                                        var lvl = modelData.level
                                        if (lvl === "SUCCESS") return "#065f46"
                                        if (lvl === "WARN") return "#854d0e"
                                        if (lvl === "ERROR") return "#991b1b"
                                        return "#1e3a8a"
                                    }
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.level
                                        color: "#ffffff"
                                        font.bold: true
                                        font.pixelSize: 9
                                    }
                                }

                                Text {
                                    text: "[" + modelData.category + "]"
                                    color: "#38bdf8"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                Text {
                                    text: modelData.message
                                    color: {
                                        var lvl = modelData.level
                                        if (lvl === "ERROR") return "#f87171"
                                        if (lvl === "WARN") return "#fde047"
                                        if (lvl === "SUCCESS") return "#34d399"
                                        return "#cbd5e1"
                                    }
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar { active: true }
                    }
                }
            }
        }
    }

    // Status Footer Bar
    footer: Rectangle {
        height: 28
        color: "#0284c7"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            Text {
                text: "状态: " + projectController.statusMessage
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 11
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "MSVC x64 + Qt 6.8.3 QML + Graphviz 拓扑分析引擎"
                color: "#e0f2fe"
                font.pixelSize: 11
            }
        }
    }

    // Dialog: Network Diagnostic
    Dialog {
        id: netDialog
        title: "代码托管平台网络连通性诊断"
        modal: true
        anchors.centerIn: parent
        width: 640
        height: 500
        background: Rectangle {
            color: "#1e293b"
            radius: 8
            border.color: "#334155"
            border.width: 1
        }

        header: Rectangle {
            height: 48
            color: "#0f172a"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Text {
                    text: "🌐 代码托管平台网络连通性诊断中心"
                    color: "#f8fafc"
                    font.bold: true
                    font.pixelSize: 14
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: networkTester.testing ? "诊断中..." : "重新探测全部"
                    enabled: !networkTester.testing
                    background: Rectangle {
                        color: "#0284c7"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 11
                    }
                    onClicked: networkTester.testAllPlatforms()
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: 12

            ListModel {
                id: netResultsModel
                ListElement { platform: "GitHub"; key: "github"; url: "https://github.com"; status: "未探测"; latency: "-"; ok: false }
                ListElement { platform: "Gitee (码云)"; key: "gitee"; url: "https://gitee.com"; status: "未探测"; latency: "-"; ok: false }
                ListElement { platform: "GitLab"; key: "gitlab"; url: "https://gitlab.com"; status: "未探测"; latency: "-"; ok: false }
                ListElement { platform: "GitCode (CSDN)"; key: "gitcode"; url: "https://gitcode.com"; status: "未探测"; latency: "-"; ok: false }
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
                    height: 68
                    color: "#0f172a"
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
                            color: model.ok ? "#10b981" : (model.status === "未探测" ? "#64748b" : "#ef4444")
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Text {
                                    text: model.platform
                                    color: "#f8fafc"
                                    font.bold: true
                                    font.pixelSize: 13
                                }
                                Text {
                                    text: "(" + model.url + ")"
                                    color: "#94a3b8"
                                    font.pixelSize: 11
                                }
                            }
                            Text {
                                text: model.status
                                color: model.ok ? "#34d399" : (model.status === "未探测" ? "#94a3b8" : "#f87171")
                                font.pixelSize: 11
                            }
                        }

                        Text {
                            text: model.latency
                            color: "#38bdf8"
                            font.bold: true
                            font.pixelSize: 13
                        }
                    }
                }
            }
        }

        onOpened: {
            networkTester.testAllPlatforms()
        }
    }

    // Dialog: Help & Documentation
    Dialog {
        id: helpDialog
        title: "工具使用说明与架构指南"
        modal: true
        anchors.centerIn: parent
        width: 680
        height: 520
        background: Rectangle {
            color: "#1e293b"
            radius: 8
            border.color: "#334155"
        }

        header: Rectangle {
            height: 48
            color: "#0f172a"
            radius: 8
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Text {
                    text: "📖 CodeInsight Pro 工具使用指南"
                    color: "#f8fafc"
                    font.bold: true
                    font.pixelSize: 14
                }
            }
        }

        contentItem: ScrollView {
            clip: true
            TextArea {
                readOnly: true
                textFormat: TextEdit.MarkdownText
                color: "#e2e8f0"
                font.pixelSize: 12
                background: Item {}
                padding: 16
                text: "### 💡 核心功能指南\n\n" +
                      "1. **远程仓库阅读**：在顶部输入框粘贴 GitHub、Gitee、GitLab 的仓库链接，点击「拉取分析」，系统将自动在后台进行 Shallow Clone 并解析全景拓扑。\n" +
                      "2. **本地工程阅读**：点击「打开本地代码」，选择任意已有项目的源码文件夹。\n" +
                      "3. **交互式拓扑图**：\n" +
                      "   - **拖拽**：按住鼠标左键可任意平移画布。\n" +
                      "   - **缩放**：滚动鼠标滚轮可平滑放大与缩小拓扑图。\n" +
                      "   - **节点选择**：单击图元卡片可查看其入度与出度。\n" +
                      "   - **双击联动**：双击图元可在上方代码编辑器中直接定位与打开文件。\n" +
                      "   - **图谱模式**：可无缝切换「全项目依赖网络」与「当前文件内部符号结构图」。\n" +
                      "4. **Scintilla 风格代码阅读**：支持行号槽滚动对齐与 C/C++/QML/Python 语法高亮。\n" +
                      "5. **网络诊断中心**：在拉取海外或私有仓库前，点击「网络连通性诊断」可测试各平台延迟与可用性。\n" +
                      "6. **全景总结与导出**：左侧「全景总结报告」提供了代码行数、语言比例及耦合度统计，并支持一键导出 Markdown 报告。"
            }
        }
    }

    // Export Confirmation Notice Dialog
    Dialog {
        id: exportNotice
        title: "导出成功"
        anchors.centerIn: parent
        width: 380
        height: 160
        modal: true
        background: Rectangle {
            color: "#1e293b"
            radius: 8
            border.color: "#10b981"
        }
        contentItem: ColumnLayout {
            spacing: 12
            Text {
                text: "✅ 分析报告已成功导出！"
                color: "#34d399"
                font.bold: true
                font.pixelSize: 14
            }
            Text {
                text: "保存路径为项目根目录下的:\nPROJECT_ANALYSIS_REPORT.md"
                color: "#cbd5e1"
                font.pixelSize: 11
            }
            Button {
                text: "确定"
                Layout.alignment: Qt.AlignRight
                background: Rectangle { color: "#0284c7"; radius: 4 }
                contentItem: Text { text: "确定"; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter }
                onClicked: exportNotice.close()
            }
        }
    }
}

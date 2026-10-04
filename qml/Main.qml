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
    color: isDarkMode ? "#0f172a" : "#f8fafc"

    // Theme properties
    property bool isDarkMode: true
    property color cBackground: isDarkMode ? "#0f172a" : "#f1f5f9"
    property color cSurface: isDarkMode ? "#1e293b" : "#ffffff"
    property color cBorder: isDarkMode ? "#334155" : "#e2e8f0"
    property color cText: isDarkMode ? "#f8fafc" : "#0f172a"
    property color cSubText: isDarkMode ? "#94a3b8" : "#64748b"
    property color cEditorBg: isDarkMode ? "#1e1e24" : "#ffffff"

    property string currentSelectedFilePath: ""
    property string currentFileExtension: "cpp"
    property bool isFileGraphActive: true
    property string currentFolderFilter: ""

    // Standalone Pop-up Windows
    GraphWindow {
        id: popupGraphWin
        dotSource: isFileGraphActive ? projectController.fileGraphDot : projectController.currentSymbolGraphDot
        isFileGraph: isFileGraphActive
        themeBackground: cBackground
        themeCard: cSurface
        themeBorder: cBorder
        themeText: cText
        onNodeDoubleClicked: (nodeId, fullPath) => {
            currentSelectedFilePath = fullPath;
            currentFileExtension = fullPath.split('.').pop();
            codeEditor.text = projectController.readFileContent(fullPath);
            editorBridge.fileExtension = currentFileExtension;
            appLogger.logInfo("独立窗口联动", "双击定位并打开代码: " + fullPath);
        }
    }

    ReportWindow {
        id: popupReportWin
        projectSummary: projectController.projectSummary
        themeBackground: cBackground
        themeCard: cSurface
        themeBorder: cBorder
        themeText: cText
    }

    // Menu Bar
    menuBar: MenuBar {
        background: Rectangle {
            color: cSurface
            border.color: cBorder
            border.width: 1
        }

        Menu {
            title: "文件 (F)"
            MenuItem {
                text: "📂 打开本地代码文件夹..."
                onTriggered: folderDialog.open()
            }
            MenuItem {
                text: "🔄 重新分析当前工程"
                onTriggered: {
                    appLogger.logInfo("菜单操作", "执行重新分析工程");
                    projectController.refreshAnalysis();
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "🚪 退出"
                onTriggered: Qt.quit()
            }
        }

        Menu {
            title: "分析与视图 (V)"
            MenuItem {
                text: "🌐 打开独立拓扑图窗口 (Pop-out)"
                onTriggered: {
                    popupGraphWin.dotSource = isFileGraphActive ? projectController.fileGraphDot : projectController.currentSymbolGraphDot;
                    popupGraphWin.show();
                    popupGraphWin.raise();
                    appLogger.logInfo("视图操作", "打开独立拓扑图窗口");
                }
            }
            MenuItem {
                text: "📋 打开独立全景总结报告窗口 (Pop-out)"
                onTriggered: {
                    popupReportWin.projectSummary = projectController.projectSummary;
                    popupReportWin.show();
                    popupReportWin.raise();
                    appLogger.logInfo("视图操作", "打开独立全景总结报告窗口");
                }
            }
            MenuSeparator {}
            MenuItem {
                text: isDarkMode ? "☀️ 切换至明亮模式 (Light Theme)" : "🌙 切换至深色模式 (Dark Theme)"
                onTriggered: {
                    isDarkMode = !isDarkMode;
                    appLogger.logInfo("主题切换", isDarkMode ? "已切换至专业深色主题" : "已切换至清晰明亮主题");
                }
            }
        }

        Menu {
            title: "工具 (T)"
            MenuItem {
                text: "🌐 代码托管平台网络连通性诊断..."
                onTriggered: {
                    appLogger.logInfo("工具操作", "打开网络连通性诊断");
                    netDialog.open();
                }
            }
            MenuItem {
                text: "💾 快速导出全景总结报告 (Markdown)..."
                onTriggered: {
                    var p = projectController.currentProjectPath + "/PROJECT_ANALYSIS_REPORT.md";
                    if (projectController.exportReport(p)) {
                        exportNotice.open();
                    }
                }
            }
        }

        Menu {
            title: "帮助 (H)"
            MenuItem {
                text: "📖 使用指南与操作说明"
                onTriggered: helpDialog.open()
            }
            MenuItem {
                text: "ℹ️ 关于 CodeInsight Pro"
                onTriggered: aboutDialog.open()
            }
        }
    }

    // Top Command / Navigation Bar
    header: ToolBar {
        height: 52
        background: Rectangle {
            color: cSurface
            border.color: cBorder
            border.width: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Brand
            RowLayout {
                spacing: 8
                Rectangle {
                    width: 32
                    height: 32
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
                        color: cText
                        font.bold: true
                        font.pixelSize: 14
                    }
                    Text {
                        text: "工作区: ./workspace (程序本地目录)"
                        color: cSubText
                        font.pixelSize: 10
                    }
                }
            }

            Rectangle { width: 1; height: 26; color: cBorder }

            // URL Input Box with Repo Icon
            Rectangle {
                Layout.fillWidth: true
                Layout.maximumWidth: 500
                height: 36
                color: isDarkMode ? "#0f172a" : "#f1f5f9"
                radius: 6
                border.color: repoInput.activeFocus ? "#38bdf8" : cBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 6

                    Text { text: "🔗"; color: cSubText; font.pixelSize: 12 }

                    TextField {
                        id: repoInput
                        Layout.fillWidth: true
                        placeholderText: "输入 GitHub / Gitee / GitLab 仓库链接..."
                        color: cText
                        placeholderTextColor: cSubText
                        font.pixelSize: 12
                        background: Item {}
                        selectByMouse: true
                    }

                    Button {
                        text: projectController.isBusy ? "拉取中..." : "拉取分析"
                        enabled: !projectController.isBusy && repoInput.text.trim().length > 0
                        implicitHeight: 28
                        background: Rectangle {
                            color: parent.enabled ? (parent.down ? "#0284c7" : "#0284c7") : (isDarkMode ? "#334155" : "#cbd5e1")
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
                            appLogger.logInfo("用户操作", "发起拉取仓库: " + repoInput.text.trim());
                            projectController.cloneAndOpenRepo(repoInput.text);
                        }
                    }
                }
            }

            // Buttons
            Button {
                text: "📂 打开本地代码"
                implicitHeight: 34
                background: Rectangle {
                    color: parent.down ? cBorder : cSurface
                    border.color: cBorder
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: cText
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: folderDialog.open()
            }

            Button {
                text: "🌐 弹出关系图"
                implicitHeight: 34
                background: Rectangle {
                    color: parent.down ? cBorder : cSurface
                    border.color: cBorder
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: cText
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    popupGraphWin.dotSource = isFileGraphActive ? projectController.fileGraphDot : projectController.currentSymbolGraphDot;
                    popupGraphWin.show();
                    popupGraphWin.raise();
                }
            }

            Button {
                text: "📋 弹出总结报告"
                implicitHeight: 34
                background: Rectangle {
                    color: parent.down ? cBorder : cSurface
                    border.color: cBorder
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: cText
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    popupReportWin.projectSummary = projectController.projectSummary;
                    popupReportWin.show();
                    popupReportWin.raise();
                }
            }

            Item { Layout.fillWidth: true }

            // Theme toggle button
            Button {
                text: isDarkMode ? "☀️" : "🌙"
                implicitHeight: 34
                implicitWidth: 38
                background: Rectangle {
                    color: cSurface
                    border.color: cBorder
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: isDarkMode = !isDarkMode
            }

            // Network diagnostic
            Button {
                text: "🌐 连通性测试"
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
                onClicked: netDialog.open()
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
        // LEFT PANE: Project Directory Explorer (Grouped / Filtered)
        // ==========================================
        Rectangle {
            SplitView.preferredWidth: 320
            SplitView.minimumWidth: 240
            SplitView.maximumWidth: 480
            color: cBackground
            border.color: cBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 42
                    color: cSurface
                    border.color: cBorder

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Text {
                            text: "📁 源码目录与文件 (" + projectController.fileList.length + ")"
                            color: cText
                            font.bold: true
                            font.pixelSize: 12
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: currentFolderFilter.length > 0 ? "全部文件" : "按目录浏览"
                            implicitHeight: 24
                            background: Rectangle {
                                color: currentFolderFilter.length > 0 ? "#0284c7" : (isDarkMode ? "#334155" : "#e2e8f0")
                                radius: 3
                            }
                            contentItem: Text {
                                text: parent.text
                                color: currentFolderFilter.length > 0 ? "#ffffff" : cText
                                font.pixelSize: 10
                            }
                            onClicked: {
                                if (currentFolderFilter.length > 0) {
                                    currentFolderFilter = "";
                                } else {
                                    folderBrowseDialog.open();
                                }
                            }
                        }
                    }
                }

                // Filter Bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    color: isDarkMode ? "#131b2e" : "#f8fafc"
                    border.color: cBorder

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Text { text: "🔍"; color: cSubText; font.pixelSize: 11 }

                        TextField {
                            id: fileFilterInput
                            Layout.fillWidth: true
                            placeholderText: currentFolderFilter.length > 0 ? ("已过滤: " + currentFolderFilter) : "快速搜索文件名或路径..."
                            color: cText
                            placeholderTextColor: cSubText
                            font.pixelSize: 11
                            background: Item {}
                        }

                        Button {
                            text: "×"
                            visible: fileFilterInput.text.length > 0 || currentFolderFilter.length > 0
                            implicitWidth: 20
                            implicitHeight: 20
                            background: Item {}
                            contentItem: Text { text: "×"; color: cSubText; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                            onClicked: {
                                fileFilterInput.text = "";
                                currentFolderFilter = "";
                            }
                        }
                    }
                }

                // File List View
                ListView {
                    id: fileListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: {
                        var q = fileFilterInput.text.toLowerCase().trim();
                        var fDir = currentFolderFilter.toLowerCase();
                        var res = [];
                        for (var i = 0; i < projectController.fileList.length; ++i) {
                            var item = projectController.fileList[i];
                            var pathLower = item.path.toLowerCase();
                            if (fDir.length > 0 && !pathLower.startsWith(fDir)) continue;
                            if (q.length > 0 && item.name.toLowerCase().indexOf(q) === -1 && pathLower.indexOf(q) === -1) continue;
                            res.push(item);
                        }
                        return res;
                    }

                    delegate: Rectangle {
                        width: fileListView.width
                        height: 46
                        color: currentSelectedFilePath === modelData.path ? (isDarkMode ? "#1e293b" : "#e0f2fe") : (maItem.containsMouse ? (isDarkMode ? "#172033" : "#f1f5f9") : "transparent")
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

                            // Badge
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
                                    color: currentSelectedFilePath === modelData.path ? "#38bdf8" : cText
                                    font.bold: currentSelectedFilePath === modelData.path
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData.path + " • " + modelData.lines + "行 • 依赖 " + modelData.dependenciesCount
                                    color: cSubText
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
        }

        // ==========================================
        // CENTER & RIGHT: Code Viewer + Topology Visualizer + Logs
        // ==========================================
        SplitView {
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            // TOP SECTION: Code Editor
            Rectangle {
                SplitView.preferredHeight: 460
                SplitView.minimumHeight: 200
                SplitView.fillWidth: true
                color: cEditorBg

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Editor Header Bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        color: cSurface
                        border.color: cBorder
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: currentSelectedFilePath.length > 0 ? ("📄 " + currentSelectedFilePath) : "未选择任何代码文件"
                                color: cText
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Rectangle {
                                width: 50
                                height: 20
                                radius: 3
                                color: isDarkMode ? "#334155" : "#e2e8f0"
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
                                    color: isDarkMode ? "#334155" : "#e2e8f0"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: cText
                                    font.pixelSize: 11
                                }
                                onClicked: {
                                    projectController.selectFile(currentSelectedFilePath);
                                    topoGraphView.selectNode(currentSelectedFilePath);
                                }
                            }

                            Text {
                                text: "QSyntaxHighlighter 驱动 | UTF-8"
                                color: cSubText
                                font.pixelSize: 10
                            }
                        }
                    }

                    // Editor Canvas with Line Numbers
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        Rectangle {
                            Layout.fillHeight: true
                            width: 52
                            color: isDarkMode ? "#18181b" : "#f1f5f9"
                            border.color: cBorder

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
                                        color: cSubText
                                        font.family: "Consolas, 'Cascadia Code', monospace"
                                        font.pixelSize: 12
                                    }
                                }
                            }
                        }

                        ScrollView {
                            id: codeScrollView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: codeEditor
                                font.family: "Consolas, 'Cascadia Code', 'Courier New', monospace"
                                font.pixelSize: 13
                                color: isDarkMode ? "#d4d4d4" : "#1e293b"
                                selectionColor: "#264f78"
                                selectedTextColor: "#ffffff"
                                selectByMouse: true
                                wrapMode: TextArea.NoWrap
                                background: Rectangle { color: cEditorBg }
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

            // MIDDLE SECTION: Topology Visualizer
            Rectangle {
                SplitView.fillHeight: true
                SplitView.preferredHeight: 360
                SplitView.minimumHeight: 180
                SplitView.fillWidth: true
                color: isDarkMode ? "#121216" : "#f8fafc"
                border.color: cBorder

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Graph Toolbar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 42
                        color: cSurface
                        border.color: cBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                text: "📊 依赖拓扑图 (Graphviz & QPainter)"
                                color: cText
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Rectangle { width: 1; height: 18; color: cBorder }

                            Button {
                                text: "全项目文件依赖网络"
                                implicitHeight: 28
                                background: Rectangle {
                                    color: isFileGraphActive ? "#0284c7" : (isDarkMode ? "#1e293b" : "#f1f5f9")
                                    border.color: isFileGraphActive ? "#38bdf8" : cBorder
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: isFileGraphActive ? "#ffffff" : cText
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
                                    color: !isFileGraphActive ? "#0284c7" : (isDarkMode ? "#1e293b" : "#f1f5f9")
                                    border.color: !isFileGraphActive ? "#38bdf8" : cBorder
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: !isFileGraphActive ? "#ffffff" : cText
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
                                text: "🗖 弹出独立窗口"
                                implicitHeight: 28
                                background: Rectangle {
                                    color: isDarkMode ? "#334155" : "#e2e8f0"
                                    border.color: cBorder
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: cText
                                    font.pixelSize: 11
                                }
                                onClicked: {
                                    popupGraphWin.dotSource = isFileGraphActive ? projectController.fileGraphDot : projectController.currentSymbolGraphDot;
                                    popupGraphWin.show();
                                    popupGraphWin.raise();
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: topoGraphView.statusMessage
                                color: cSubText
                                font.pixelSize: 11
                            }

                            Button { text: "➕"; implicitWidth: 32; implicitHeight: 28; onClicked: topoGraphView.zoomIn() }
                            Button { text: "➖"; implicitWidth: 32; implicitHeight: 28; onClicked: topoGraphView.zoomOut() }
                            Button { text: "适应窗口"; implicitHeight: 28; onClicked: topoGraphView.fitToView() }
                            Button { text: "复位"; implicitHeight: 28; onClicked: topoGraphView.resetView() }
                        }
                    }

                    // Interactive Graphviz View
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

                                onPressed: (mouse) => {
                                    lastX = mouse.x
                                    lastY = mouse.y
                                    topoGraphView.handleMousePress(mouse.x, mouse.y)
                                }

                                onPositionChanged: (mouse) => {
                                    topoGraphView.handleMouseMove(mouse.x, mouse.y)
                                    if (mouse.buttons & Qt.LeftButton || mouse.buttons & Qt.MiddleButton) {
                                        var dx = mouse.x - lastX
                                        var dy = mouse.y - lastY
                                        topoGraphView.panBy(dx, dy)
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
                color: isDarkMode ? "#090d16" : "#f8fafc"
                border.color: cBorder

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: cSurface
                        border.color: cBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: "📜 运行操作日志 (" + appLogger.count + " 条记录)"
                                color: cText
                                font.bold: true
                                font.pixelSize: 11
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "清空日志"
                                implicitHeight: 22
                                background: Rectangle {
                                    color: isDarkMode ? "#334155" : "#e2e8f0"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: cText
                                    font.pixelSize: 10
                                }
                                onClicked: appLogger.clear()
                            }
                        }
                    }

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
                            color: index % 2 === 0 ? (isDarkMode ? "#090d16" : "#ffffff") : (isDarkMode ? "#0d131f" : "#f1f5f9")

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Text {
                                    text: modelData.timestamp
                                    color: cSubText
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
                                        if (lvl === "ERROR") return "#ef4444"
                                        if (lvl === "WARN") return "#eab308"
                                        if (lvl === "SUCCESS") return "#10b981"
                                        return cText
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
                text: "MSVC x64 + Qt 6.8.3 QML + 3rdparty/graphviz 拓扑分析引擎"
                color: "#e0f2fe"
                font.pixelSize: 11
            }
        }
    }

    // Dialog: Network Diagnostic (with explicit close button)
    Dialog {
        id: netDialog
        title: "代码托管平台网络连通性诊断"
        modal: true
        anchors.centerIn: parent
        width: 640
        height: 520
        background: Rectangle {
            color: cSurface
            radius: 8
            border.color: cBorder
            border.width: 1
        }

        header: Rectangle {
            height: 48
            color: isDarkMode ? "#0f172a" : "#f1f5f9"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Text {
                    text: "🌐 代码托管平台网络连通性诊断中心"
                    color: cText
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
                    contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; font.pixelSize: 11 }
                    onClicked: networkTester.testAllPlatforms()
                }
                Button {
                    text: "关闭 ✕"
                    background: Rectangle {
                        color: isDarkMode ? "#334155" : "#e2e8f0"
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: cText; font.pixelSize: 11 }
                    onClicked: netDialog.close()
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
                    color: isDarkMode ? "#0f172a" : "#ffffff"
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
                            color: model.ok ? "#10b981" : (model.status === "未探测" ? cSubText : "#ef4444")
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Text {
                                    text: model.platform
                                    color: cText
                                    font.bold: true
                                    font.pixelSize: 13
                                }
                                Text {
                                    text: "(" + model.url + ")"
                                    color: cSubText
                                    font.pixelSize: 11
                                }
                            }
                            Text {
                                text: model.status
                                color: model.ok ? "#34d399" : (model.status === "未探测" ? cSubText : "#f87171")
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

            // Bottom close button
            Button {
                text: "完成并关闭"
                Layout.alignment: Qt.AlignRight
                background: Rectangle { color: "#0284c7"; radius: 4 }
                contentItem: Text { text: parent.text; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter }
                onClicked: netDialog.close()
            }
        }

        onOpened: {
            networkTester.testAllPlatforms()
        }
    }

    // Dialog: Help & Documentation (with explicit close button)
    Dialog {
        id: helpDialog
        title: "工具使用说明"
        modal: true
        anchors.centerIn: parent
        width: 700
        height: 540
        background: Rectangle {
            color: cSurface
            radius: 8
            border.color: cBorder
        }

        header: Rectangle {
            height: 48
            color: isDarkMode ? "#0f172a" : "#f1f5f9"
            radius: 8
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Text {
                    text: "📖 CodeInsight Pro 工具使用指南"
                    color: cText
                    font.bold: true
                    font.pixelSize: 14
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "关闭 ✕"
                    background: Rectangle {
                        color: isDarkMode ? "#334155" : "#e2e8f0"
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: cText; font.pixelSize: 11 }
                    onClicked: helpDialog.close()
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: 8
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                TextArea {
                    readOnly: true
                    textFormat: TextEdit.MarkdownText
                    color: cText
                    font.pixelSize: 12
                    background: Item {}
                    padding: 16
                    wrapMode: TextArea.Wrap
                    text: "### 💡 核心功能与操作说明\n\n" +
                          "1. **远程仓库阅读**：\n" +
                          "   - 在顶部链接框输入 GitHub / Gitee / GitLab / GitCode 链接后点击「拉取分析」。\n" +
                          "   - 仓库代码将存放于软件运行目录下的 `workspace` 文件夹中，避免占用系统 C 盘空间。\n\n" +
                          "2. **源码按目录层级分类查看**：\n" +
                          "   - 左侧面板支持输入关键字过滤文件，或点击「按目录浏览」按项目子文件夹进行精准定位。\n\n" +
                          "3. **拓扑关系图独立窗口**：\n" +
                          "   - 点击顶栏或菜单栏「弹出关系图」，即可在独立的超大窗口中全屏查看与交互拓扑图。\n" +
                          "   - 支持鼠标滚轮缩放、拖拽平移、图元点击高亮连线、双击联动跳转代码。\n\n" +
                          "4. **全景总结报告独立窗口与多格式导出**：\n" +
                          "   - 点击「弹出总结报告」打开独立分析报告窗口。\n" +
                          "   - 支持一键导出为 **Word 标准文档 (.doc)**、**HTML 网页 (.html)** 或 **Markdown 文档 (.md)**。\n\n" +
                          "5. **界面主题切换**：\n" +
                          "   - 菜单栏或顶栏太阳/月亮图标支持即时切换深色 (Dark) / 明亮 (Light) 专业主题。\n\n" +
                          "6. **Graphviz 引擎保障**：\n" +
                          "   - Graphviz 源码已拉取至工程目录 `3rdparty/graphviz`，以便后续持续打包与集成。"
                }
            }

            Button {
                text: "我知道了 (关闭)"
                Layout.alignment: Qt.AlignRight
                background: Rectangle { color: "#0284c7"; radius: 4 }
                contentItem: Text { text: parent.text; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter }
                onClicked: helpDialog.close()
            }
        }
    }

    // Dialog: About Dialog
    Dialog {
        id: aboutDialog
        title: "关于 CodeInsight Pro"
        anchors.centerIn: parent
        width: 440
        height: 220
        modal: true
        background: Rectangle {
            color: cSurface
            radius: 8
            border.color: cBorder
        }
        contentItem: ColumnLayout {
            spacing: 12
            Text {
                text: "CodeInsight Pro v2.0"
                color: "#38bdf8"
                font.bold: true
                font.pixelSize: 16
            }
            Text {
                text: "基于 C++ 17 + MSVC 2022 + Qt 6.8.3 QML 构建\n集成了 Graphviz 拓扑分析与 QSyntaxHighlighter 语法引擎"
                color: cText
                font.pixelSize: 12
            }
            Item { Layout.fillHeight: true }
            Button {
                text: "关闭"
                Layout.alignment: Qt.AlignRight
                background: Rectangle { color: "#0284c7"; radius: 4 }
                contentItem: Text { text: "关闭"; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter }
                onClicked: aboutDialog.close()
            }
        }
    }

    // Notice Dialog
    Dialog {
        id: exportNotice
        title: "导出成功"
        anchors.centerIn: parent
        width: 380
        height: 160
        modal: true
        background: Rectangle {
            color: cSurface
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
                color: cText
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

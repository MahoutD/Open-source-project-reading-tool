import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: reportWin
    width: 960
    height: 720
    minimumWidth: 700
    minimumHeight: 500
    title: "📊 项目全景架构与依赖分析报告"
    color: themeBackground

    property string projectSummary: ""
    property color themeBackground: "#0f172a"
    property color themeCard: "#1e293b"
    property color themeBorder: "#334155"
    property color themeText: "#f8fafc"

    FileDialog {
        id: saveDocDialog
        title: "导出 Word 报告"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Word 文档 (*.doc)", "所有文件 (*.*)"]
        onAccepted: {
            projectController.exportReportDoc(selectedFile.toString())
        }
    }

    FileDialog {
        id: saveHtmlDialog
        title: "导出 HTML 报告"
        fileMode: FileDialog.SaveFile
        nameFilters: ["HTML 页面 (*.html)", "所有文件 (*.*)"]
        onAccepted: {
            projectController.exportReportHtml(selectedFile.toString())
        }
    }

    FileDialog {
        id: saveMdDialog
        title: "导出 Markdown 报告"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Markdown 文件 (*.md)", "所有文件 (*.*)"]
        onAccepted: {
            projectController.exportReport(selectedFile.toString())
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: themeCard
            border.color: themeBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "📋 项目宏观概况与多维度统计全景报告"
                    color: themeText
                    font.bold: true
                    font.pixelSize: 13
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "📄 导出 Word (.doc)"
                    implicitHeight: 30
                    background: Rectangle {
                        color: "#2563eb"
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; font.pixelSize: 11 }
                    onClicked: saveDocDialog.open()
                }

                Button {
                    text: "🌐 导出 HTML (.html)"
                    implicitHeight: 30
                    background: Rectangle {
                        color: "#059669"
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; font.pixelSize: 11 }
                    onClicked: saveHtmlDialog.open()
                }

                Button {
                    text: "📝 导出 Markdown (.md)"
                    implicitHeight: 30
                    background: Rectangle {
                        color: "#0284c7"
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; font.pixelSize: 11 }
                    onClicked: saveMdDialog.open()
                }

                Button {
                    text: "关闭"
                    implicitHeight: 30
                    background: Rectangle {
                        color: "#475569"
                        radius: 4
                    }
                    contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; font.pixelSize: 11 }
                    onClicked: reportWin.close()
                }
            }
        }

        // Report Body
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            TextArea {
                readOnly: true
                text: reportWin.projectSummary
                textFormat: TextEdit.MarkdownText
                color: "#e2e8f0"
                font.pixelSize: 13
                background: Rectangle { color: themeBackground }
                padding: 24
                wrapMode: TextArea.Wrap
                selectByMouse: true
            }
        }
    }
}

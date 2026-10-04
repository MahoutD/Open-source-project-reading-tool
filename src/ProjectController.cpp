#include "ProjectController.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>

ProjectController::ProjectController(QObject *parent)
    : QObject(parent)
{
}

void ProjectController::openLocalFolder(const QString &folderPath) {
    QString path = folderPath;
    if (path.startsWith("file:///")) {
        path = QUrl(folderPath).toLocalFile();
    }
#ifdef Q_OS_WIN
    if (path.startsWith("/") && path.length() > 2 && path[2] == ':') {
        path = path.mid(1); // Strip leading / if /D:/...
    }
#endif
    path = QDir::cleanPath(path);

    QDir dir(path);
    if (!dir.exists()) {
        m_statusMessage = "错误：目录不存在 " + path;
        Logger::instance()->logError("项目加载", m_statusMessage);
        emit statusMessageChanged();
        return;
    }

    Logger::instance()->logInfo("项目加载", "打开本地工作区: " + path);
    m_currentProjectPath = path;
    emit currentProjectPathChanged();
    refreshAnalysis();
}

void ProjectController::cloneAndOpenRepo(const QString &repoUrl) {
    QString trimmed = repoUrl.trimmed();
    if (trimmed.isEmpty()) return;

    m_busy = true;
    emit isBusyChanged();
    m_statusMessage = "正在拉取仓库代码: " + trimmed + " ...";
    Logger::instance()->logInfo("Git克隆", "发起远程克隆: " + trimmed);
    emit statusMessageChanged();

    // Extract repo name
    QString repoName = trimmed.section('/', -1);
    if (repoName.endsWith(".git")) {
        repoName.chop(4);
    }
    if (repoName.isEmpty()) repoName = "repo_cloned";

    QString tempBase = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/OpenSourceReader";
    QDir().mkpath(tempBase);
    QString targetDir = tempBase + "/" + repoName;

    // Run git clone asynchronously
    (void)QtConcurrent::run([this, trimmed, targetDir, repoName]() {
        if (QDir(targetDir).exists()) {
            Logger::instance()->logInfo("Git拉取", "本地已存在仓库缓存，尝试执行 git pull...");
            QProcess pullProc;
            pullProc.setWorkingDirectory(targetDir);
            pullProc.start("git", QStringList() << "pull");
            pullProc.waitForFinished(30000);
        } else {
            Logger::instance()->logInfo("Git拉取", QString("正在执行: git clone --depth 1 %1 %2").arg(trimmed, targetDir));
            QProcess cloneProc;
            cloneProc.start("git", QStringList() << "clone" << "--depth" << "1" << trimmed << targetDir);
            if (!cloneProc.waitForFinished(60000)) {
                QMetaObject::invokeMethod(this, [this]() {
                    m_busy = false;
                    emit isBusyChanged();
                    m_statusMessage = "克隆超时或网络失败，请检查网络联通性。";
                    Logger::instance()->logError("Git拉取", m_statusMessage);
                    emit statusMessageChanged();
                    emit cloneFinished(false, m_statusMessage);
                });
                return;
            }
            if (cloneProc.exitCode() != 0) {
                QString err = cloneProc.readAllStandardError();
                QMetaObject::invokeMethod(this, [this, err]() {
                    m_busy = false;
                    emit isBusyChanged();
                    m_statusMessage = "Git 克隆失败: " + err;
                    Logger::instance()->logError("Git拉取", m_statusMessage);
                    emit statusMessageChanged();
                    emit cloneFinished(false, m_statusMessage);
                });
                return;
            }
        }

        QMetaObject::invokeMethod(this, [this, targetDir, repoName]() {
            m_busy = false;
            emit isBusyChanged();
            m_statusMessage = "代码拉取成功，正在解析结构...";
            Logger::instance()->logSuccess("Git克隆", QString("代码仓库 [%1] 拉取完成").arg(repoName));
            emit statusMessageChanged();
            emit cloneFinished(true, "拉取成功");
            openLocalFolder(targetDir);
        });
    });
}

void ProjectController::refreshAnalysis() {
    if (m_currentProjectPath.isEmpty()) return;

    m_busy = true;
    emit isBusyChanged();
    m_statusMessage = "正在分析代码依赖与结构图谱...";
    Logger::instance()->logInfo("代码分析", "开始遍历工作区代码与构建拓扑图...");
    emit statusMessageChanged();

    QString projectPath = m_currentProjectPath;

    (void)QtConcurrent::run([this, projectPath]() {
        QMap<QString, FileDependencyInfo> map = DependencyAnalyzer::analyzeProject(projectPath);
        QString fileDot = DependencyAnalyzer::generateDotFileGraph(map);

        QVariantList list;
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            QVariantMap item;
            item["path"] = it.key();
            item["name"] = it.value().fileName;
            item["type"] = it.value().fileType;
            item["size"] = it.value().fileSize;
            item["lines"] = it.value().lineCount;
            item["dependenciesCount"] = it.value().resolvedDependencies.size();
            list.append(item);
        }

        QMetaObject::invokeMethod(this, [this, map, fileDot, list]() {
            m_analyzedFiles = map;
            m_fileGraphDot = fileDot;
            m_fileList = list;
            emit fileGraphDotChanged();
            emit fileListChanged();

            generateSummary();

            m_busy = false;
            emit isBusyChanged();
            m_statusMessage = QString("分析完成，共识别 %1 个源码文件").arg(map.size());
            Logger::instance()->logSuccess("代码分析", m_statusMessage);
            emit statusMessageChanged();
        });
    });
}

void ProjectController::selectFile(const QString &filePath) {
    if (!m_analyzedFiles.contains(filePath)) return;
    const FileDependencyInfo &info = m_analyzedFiles[filePath];
    m_currentSymbolGraphDot = DependencyAnalyzer::generateDotSymbolGraph(filePath, info);
    Logger::instance()->logInfo("文件浏览", QString("激活文件: %1 (%2 行代码, %3 个内部符号)")
                                              .arg(info.fileName)
                                              .arg(info.lineCount)
                                              .arg(info.symbols.size()));
    emit currentSymbolGraphDotChanged();
}

QString ProjectController::readFileContent(const QString &relativeOrFullPath) {
    QString fullPath = relativeOrFullPath;
    if (!QFileInfo(fullPath).isAbsolute()) {
        fullPath = m_currentProjectPath + "/" + relativeOrFullPath;
    }

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::instance()->logWarning("文件读取", "无法读取文件: " + fullPath);
        return "// 无法打开文件: " + fullPath;
    }
    return QString::fromUtf8(file.readAll());
}

void ProjectController::generateSummary() {
    int totalFiles = m_analyzedFiles.size();
    int totalLines = 0;
    qint64 totalBytes = 0;
    QMap<QString, int> typeCounts;
    QMap<QString, int> typeLines;

    for (const auto &info : m_analyzedFiles) {
        totalLines += info.lineCount;
        totalBytes += info.fileSize;
        typeCounts[info.fileType]++;
        typeLines[info.fileType] += info.lineCount;
    }

    QString summary = QString("### 📊 项目宏观概况与代码统计\n\n");
    summary += QString("- **项目根路径**: `%1`\n").arg(m_currentProjectPath);
    summary += QString("- **源码文件总数**: `%1` 个\n").arg(totalFiles);
    summary += QString("- **有效代码总行数**: `%1` 行\n").arg(totalLines);
    summary += QString("- **文件总大小**: `%1` KB\n\n").arg(totalBytes / 1024);

    summary += "#### 📁 语言与文件类型分布\n\n";
    summary += "| 类型/后缀 | 文件数量 | 代码行数 | 占比估算 |\n";
    summary += "| :--- | :--- | :--- | :--- |\n";

    for (auto it = typeCounts.cbegin(); it != typeCounts.cend(); ++it) {
        QString ext = it.key().isEmpty() ? "other" : it.key();
        int count = it.value();
        int lines = typeLines[it.key()];
        double pct = (totalLines > 0) ? (lines * 100.0 / totalLines) : 0.0;
        summary += QString("| `.%1` | %2 | %3 行 | %4% |\n")
                       .arg(ext).arg(count).arg(lines).arg(QString::number(pct, 'f', 1));
    }

    summary += "\n#### 🔗 关键依赖与架构概况\n\n";
    int highDepCount = 0;
    for (const auto &info : m_analyzedFiles) {
        if (info.resolvedDependencies.size() >= 2) {
            highDepCount++;
            summary += QString("- `%1`: 依赖了 %2 个模块 (%3)\n")
                           .arg(info.fileName)
                           .arg(info.resolvedDependencies.size())
                           .arg(info.resolvedDependencies.join(", "));
        }
    }
    if (highDepCount == 0) {
        summary += "- 各模块耦合度较低或属于扁平化组织结构。\n";
    }

    m_projectSummary = summary;
    emit projectSummaryChanged();
}

bool ProjectController::exportReport(const QString &targetFilePath) {
    QString path = targetFilePath;
    if (path.startsWith("file:///")) {
        path = QUrl(targetFilePath).toLocalFile();
    }
#ifdef Q_OS_WIN
    if (path.startsWith("/") && path.length() > 2 && path[2] == ':') {
        path = path.mid(1);
    }
#endif
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance()->logError("报告导出", "无法写入导出文件: " + path);
        return false;
    }
    QTextStream out(&file);
    out << m_projectSummary;
    file.close();
    Logger::instance()->logSuccess("报告导出", "项目分析总结已成功导出至: " + path);
    return true;
}

#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QThread>
#include "DependencyAnalyzer.h"

class ProjectController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString currentProjectPath READ currentProjectPath NOTIFY currentProjectPathChanged)
    Q_PROPERTY(QString projectSummary READ projectSummary NOTIFY projectSummaryChanged)
    Q_PROPERTY(QString fileGraphDot READ fileGraphDot NOTIFY fileGraphDotChanged)
    Q_PROPERTY(QString currentSymbolGraphDot READ currentSymbolGraphDot NOTIFY currentSymbolGraphDotChanged)
    Q_PROPERTY(QVariantList fileList READ fileList NOTIFY fileListChanged)

public:
    explicit ProjectController(QObject *parent = nullptr);

    bool isBusy() const { return m_busy; }
    QString statusMessage() const { return m_statusMessage; }
    QString currentProjectPath() const { return m_currentProjectPath; }
    QString projectSummary() const { return m_projectSummary; }
    QString fileGraphDot() const { return m_fileGraphDot; }
    QString currentSymbolGraphDot() const { return m_currentSymbolGraphDot; }
    QVariantList fileList() const { return m_fileList; }

    Q_INVOKABLE void openLocalFolder(const QString &folderPath);
    Q_INVOKABLE void cloneAndOpenRepo(const QString &repoUrl);
    Q_INVOKABLE QString readFileContent(const QString &relativeOrFullPath);
    Q_INVOKABLE void selectFile(const QString &filePath);
    Q_INVOKABLE void refreshAnalysis();
    Q_INVOKABLE bool exportReport(const QString &targetFilePath);

signals:
    void isBusyChanged();
    void statusMessageChanged();
    void currentProjectPathChanged();
    void projectSummaryChanged();
    void fileGraphDotChanged();
    void currentSymbolGraphDotChanged();
    void fileListChanged();
    void cloneFinished(bool success, const QString &msg);

private:
    void performAnalysis(const QString &path);
    void generateSummary();

    bool m_busy = false;
    QString m_statusMessage = "就绪";
    QString m_currentProjectPath;
    QString m_projectSummary;
    QString m_fileGraphDot;
    QString m_currentSymbolGraphDot;
    QVariantList m_fileList;

    QMap<QString, FileDependencyInfo> m_analyzedFiles;
};

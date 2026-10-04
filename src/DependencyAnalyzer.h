#pragma once
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QVariantMap>

struct InternalSymbol {
    QString name;
    QString kind; // "class", "struct", "function", "include", "var"
    int line = 1;
};

struct FileDependencyInfo {
    QString filePath;         // Relative or absolute path
    QString fileName;         // e.g. "NetworkTester.cpp"
    QString fileType;         // "cpp", "h", "qml", "py", etc.
    qint64 fileSize = 0;
    int lineCount = 0;
    QStringList includes;     // Raw #include or import targets
    QStringList resolvedDependencies; // Target file paths in this project
    QList<InternalSymbol> symbols;   // Internal definitions/calls
};

class DependencyAnalyzer {
public:
    static bool isSourceFile(const QString &ext);
    static FileDependencyInfo analyzeFile(const QString &rootPath, const QString &fullPath);
    static QMap<QString, FileDependencyInfo> analyzeProject(const QString &rootPath);

    // Graph generation
    static QString generateDotFileGraph(const QMap<QString, FileDependencyInfo> &fileMap);
    static QString generateDotSymbolGraph(const QString &filePath, const FileDependencyInfo &info);
};

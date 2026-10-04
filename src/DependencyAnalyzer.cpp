#include "DependencyAnalyzer.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QFileInfo>
#include <QSet>

bool DependencyAnalyzer::isSourceFile(const QString &ext) {
    static const QSet<QString> exts = {
        "h", "hpp", "hxx", "c", "cpp", "cxx", "cc",
        "qml", "js", "py", "java", "rs", "go", "cs", "ts"
    };
    return exts.contains(ext.toLower());
}

FileDependencyInfo DependencyAnalyzer::analyzeFile(const QString &rootPath, const QString &fullPath) {
    FileDependencyInfo info;
    QFileInfo fi(fullPath);
    info.filePath = QDir(rootPath).relativeFilePath(fullPath).replace("\\", "/");
    info.fileName = fi.fileName();
    info.fileType = fi.suffix().toLower();
    info.fileSize = fi.size();

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return info;
    }

    QTextStream in(&file);
    // Regex for includes/imports
    // C/C++ #include <...> or #include "..."
    static const QRegularExpression rxCppInc(R"(^\s*#\s*include\s*["<]([^">]+)[">])");
    // Python import x or from x import y
    static const QRegularExpression rxPyInc(R"(^\s*(?:from\s+([a-zA-Z0-9_\.]+)\s+import|import\s+([a-zA-Z0-9_\.]+)))");
    // QML import x
    static const QRegularExpression rxQmlInc(R"(^\s*import\s+([a-zA-Z0-9_\.]+))");

    // Symbol extractors for C/C++/Java/QML
    static const QRegularExpression rxClass(R"(\b(?:class|struct|interface|enum class|enum)\s+([a-zA-Z0-9_]+))");
    static const QRegularExpression rxFunc(R"(^[a-zA-Z0-9_:\*<>\s]+\s+([a-zA-Z0-9_]+)\s*\([^\)]*\)\s*(?:const)?\s*[\{;])");
    static const QRegularExpression rxQmlComp(R"(^\s*([A-Z][a-zA-Z0-9_]+)\s*\{)");

    int lineNum = 0;
    while (!in.atEnd()) {
        lineNum++;
        QString line = in.readLine();
        QString trimmed = line.trimmed();

        // 1. Check Include
        auto mCpp = rxCppInc.match(line);
        if (mCpp.hasMatch()) {
            info.includes.append(mCpp.captured(1));
            InternalSymbol sym;
            sym.name = "#include " + mCpp.captured(1);
            sym.kind = "include";
            sym.line = lineNum;
            info.symbols.append(sym);
            continue;
        }

        auto mPy = rxPyInc.match(line);
        if (mPy.hasMatch()) {
            QString mod = mPy.captured(1).isEmpty() ? mPy.captured(2) : mPy.captured(1);
            info.includes.append(mod);
            continue;
        }

        auto mQml = rxQmlInc.match(line);
        if (mQml.hasMatch()) {
            info.includes.append(mQml.captured(1));
            continue;
        }

        // 2. Check classes/structs
        auto mClass = rxClass.match(line);
        if (mClass.hasMatch()) {
            InternalSymbol sym;
            sym.name = mClass.captured(1);
            sym.kind = "class";
            sym.line = lineNum;
            info.symbols.append(sym);
            continue;
        }

        // 3. Check QML root/sub components
        if (info.fileType == "qml") {
            auto mQ = rxQmlComp.match(line);
            if (mQ.hasMatch()) {
                QString cName = mQ.captured(1);
                if (cName != "Item" && cName != "Rectangle" && cName != "Row" && cName != "Column") {
                    InternalSymbol sym;
                    sym.name = cName;
                    sym.kind = "component";
                    sym.line = lineNum;
                    info.symbols.append(sym);
                    continue;
                }
            }
        }

        // 4. Check function/method signatures
        auto mFunc = rxFunc.match(line);
        if (mFunc.hasMatch()) {
            QString name = mFunc.captured(1);
            if (name != "if" && name != "for" && name != "while" && name != "switch" && name != "catch" && name != "return") {
                InternalSymbol sym;
                sym.name = name;
                sym.kind = "function";
                sym.line = lineNum;
                info.symbols.append(sym);
            }
        }
    }

    info.lineCount = lineNum;
    return info;
}

QMap<QString, FileDependencyInfo> DependencyAnalyzer::analyzeProject(const QString &rootPath) {
    QMap<QString, FileDependencyInfo> map;
    QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    // Collect files
    while (it.hasNext()) {
        QString f = it.next();
        // Ignore git dir, build dirs, .vs, etc.
        if (f.contains("/.git/") || f.contains("/build/") || f.contains("/.vs/") || f.contains("/out/") || f.contains("/3rdparty/")) {
            continue;
        }
        QFileInfo fi(f);
        if (isSourceFile(fi.suffix())) {
            FileDependencyInfo info = analyzeFile(rootPath, f);
            map.insert(info.filePath, info);
        }
    }

    // Resolve dependencies between project files
    for (auto fileIt = map.begin(); fileIt != map.end(); ++fileIt) {
        FileDependencyInfo &info = fileIt.value();
        for (const QString &inc : info.includes) {
            QString incBase = QFileInfo(inc).fileName();
            // Match against any file in the project
            for (auto targetIt = map.cbegin(); targetIt != map.cend(); ++targetIt) {
                if (targetIt.key() == info.filePath) continue;
                if (targetIt.key().endsWith(inc) || targetIt.value().fileName == incBase) {
                    if (!info.resolvedDependencies.contains(targetIt.key())) {
                        info.resolvedDependencies.append(targetIt.key());
                    }
                }
            }
        }
    }

    return map;
}

QString DependencyAnalyzer::generateDotFileGraph(const QMap<QString, FileDependencyInfo> &fileMap) {
    QString dot = "digraph ProjectFileDependencies {\n";
    dot += "  rankdir=LR;\n";
    dot += "  node [shape=box, style=\"rounded,filled\", fillcolor=\"#1e293b\", fontcolor=\"#e2e8f0\", fontname=\"Segoe UI\", fontsize=10];\n";
    dot += "  edge [color=\"#38bdf8\", arrowhead=vee];\n\n";

    // Add nodes
    for (auto it = fileMap.cbegin(); it != fileMap.cend(); ++it) {
        QString nodeName = QString("\"%1\"").arg(it.key());
        QString label = QString("%1\\n(%2 lines)").arg(it.value().fileName).arg(it.value().lineCount);
        QString color = "#1e293b";
        if (it.value().fileType == "h" || it.value().fileType == "hpp") {
            color = "#0369a1";
        } else if (it.value().fileType == "cpp" || it.value().fileType == "c") {
            color = "#047857";
        } else if (it.value().fileType == "qml") {
            color = "#6d28d9";
        } else if (it.value().fileType == "py") {
            color = "#b45309";
        }
        dot += QString("  %1 [label=\"%2\", fillcolor=\"%3\"];\n").arg(nodeName, label, color);
    }

    dot += "\n";
    // Add edges
    for (auto it = fileMap.cbegin(); it != fileMap.cend(); ++it) {
        QString fromNode = QString("\"%1\"").arg(it.key());
        for (const QString &dep : it.value().resolvedDependencies) {
            QString toNode = QString("\"%1\"").arg(dep);
            dot += QString("  %1 -> %2;\n").arg(fromNode, toNode);
        }
    }

    dot += "}\n";
    return dot;
}

QString DependencyAnalyzer::generateDotSymbolGraph(const QString &filePath, const FileDependencyInfo &info) {
    QString dot = QString("digraph \"%1_InternalSymbols\" {\n").arg(info.fileName);
    dot += "  rankdir=LR;\n";
    dot += "  node [shape=box, style=\"rounded,filled\", fontname=\"Segoe UI\", fontsize=10];\n";
    dot += "  edge [color=\"#64748b\", arrowhead=vee];\n\n";

    QString rootId = QString("\"%1\"").arg(info.filePath);
    dot += QString("  %1 [label=\"%2\\n[%3 行代码]\", fillcolor=\"#0284c7\", fontcolor=\"#ffffff\", shape=folder];\n\n")
               .arg(rootId, info.fileName).arg(info.lineCount);

    // Group symbols by kind
    QString classGroup = "\"类与结构 (Classes)\"";
    QString funcGroup = "\"成员函数/方法 (Methods)\"";
    QString incGroup = "\"外部包含 (Includes)\"";

    bool hasClass = false, hasFunc = false, hasInc = false;
    for (const auto &s : info.symbols) {
        if (s.kind == "class") hasClass = true;
        if (s.kind == "function") hasFunc = true;
        if (s.kind == "include") hasInc = true;
    }

    if (hasClass) {
        dot += QString("  %1 [label=\"📦 类定义与结构体\", fillcolor=\"#d97706\", fontcolor=\"#ffffff\"];\n").arg(classGroup);
        dot += QString("  %1 -> %2;\n").arg(rootId, classGroup);
    }
    if (hasFunc) {
        dot += QString("  %1 [label=\"⚡ 核心函数与方法\", fillcolor=\"#059669\", fontcolor=\"#ffffff\"];\n").arg(funcGroup);
        dot += QString("  %1 -> %2;\n").arg(rootId, funcGroup);
    }
    if (hasInc) {
        dot += QString("  %1 [label=\"🔗 引用包含库\", fillcolor=\"#475569\", fontcolor=\"#ffffff\"];\n").arg(incGroup);
        dot += QString("  %1 -> %2;\n").arg(rootId, incGroup);
    }

    int idx = 0;
    for (const auto &sym : info.symbols) {
        idx++;
        QString symId = QString("\"sym_%1\"").arg(idx);
        QString color = "#334155";
        QString fColor = "#e2e8f0";
        QString parentGroup = rootId;

        if (sym.kind == "class") {
            color = "#b45309";
            fColor = "#ffffff";
            parentGroup = classGroup;
        } else if (sym.kind == "function") {
            color = "#047857";
            fColor = "#ffffff";
            parentGroup = funcGroup;
        } else if (sym.kind == "include") {
            color = "#1e293b";
            parentGroup = incGroup;
        } else if (sym.kind == "component") {
            color = "#6d28d9";
            fColor = "#ffffff";
            parentGroup = rootId;
        }

        QString label = QString("%1\\n(第 %2 行)").arg(sym.name).arg(sym.line);
        dot += QString("  %1 [label=\"%2\", fillcolor=\"%3\", fontcolor=\"%4\"];\n")
                   .arg(symId, label, color, fColor);
        dot += QString("  %1 -> %2;\n").arg(parentGroup, symId);
    }

    dot += "}\n";
    return dot;
}

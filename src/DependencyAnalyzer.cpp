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
                InternalSymbol sym;
                sym.name = mQ.captured(1);
                sym.kind = "component";
                sym.line = lineNum;
                info.symbols.append(sym);
                continue;
            }
        }

        // 4. Check function patterns (heuristics)
        auto mFunc = rxFunc.match(line);
        if (mFunc.hasMatch()) {
            QString name = mFunc.captured(1);
            if (name != "if" && name != "for" && name != "while" && name != "switch" && name != "catch") {
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
        if (f.contains("/.git/") || f.contains("/build/") || f.contains("/.vs/") || f.contains("/out/")) {
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
    dot += "  node [shape=box, style=\"rounded,filled\", fillcolor=\"#2b2d30\", fontcolor=\"#e0e0e0\", fontname=\"Segoe UI\", fontsize=10];\n";
    dot += "  edge [color=\"#5c82a6\", arrowhead=vee];\n\n";

    // Add nodes
    for (auto it = fileMap.cbegin(); it != fileMap.cend(); ++it) {
        QString nodeName = QString("\"%1\"").arg(it.key());
        QString label = QString("%1\\n(%2 lines)").arg(it.value().fileName).arg(it.value().lineCount);
        QString color = "#2b2d30";
        if (it.value().fileType == "h" || it.value().fileType == "hpp") {
            color = "#1e3a5f";
        } else if (it.value().fileType == "cpp" || it.value().fileType == "c") {
            color = "#2d4a22";
        } else if (it.value().fileType == "qml") {
            color = "#4a2d48";
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
    dot += "  rankdir=TB;\n";
    dot += "  node [shape=box, style=\"rounded,filled\", fontname=\"Segoe UI\", fontsize=10];\n";
    dot += "  edge [color=\"#6c757d\", arrowhead=vee];\n\n";

    QString rootId = "\"FileRoot\"";
    dot += QString("  %1 [label=\"%2\\n[%3 lines]\", fillcolor=\"#007acc\", fontcolor=\"#ffffff\", shape=folder];\n\n")
               .arg(rootId, info.fileName).arg(info.lineCount);

    int idx = 0;
    for (const auto &sym : info.symbols) {
        idx++;
        QString symId = QString("\"sym_%1\"").arg(idx);
        QString color = "#3c3f41";
        QString fColor = "#e0e0e0";

        if (sym.kind == "class") {
            color = "#b26900";
            fColor = "#ffffff";
        } else if (sym.kind == "function") {
            color = "#286846";
            fColor = "#ffffff";
        } else if (sym.kind == "include") {
            color = "#3a4a58";
        } else if (sym.kind == "component") {
            color = "#7d3c98";
            fColor = "#ffffff";
        }

        QString label = QString("%1 (%2)\\nline: %3").arg(sym.name, sym.kind).arg(sym.line);
        dot += QString("  %1 [label=\"%2\", fillcolor=\"%3\", fontcolor=\"%4\"];\n")
                   .arg(symId, label, color, fColor);
        dot += QString("  %1 -> %2;\n").arg(rootId, symId);
    }

    dot += "}\n";
    return dot;
}

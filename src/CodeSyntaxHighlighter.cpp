#include "CodeSyntaxHighlighter.h"

CodeSyntaxHighlighter::CodeSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupCppRules();
}

void CodeSyntaxHighlighter::setFileType(const QString &ext) {
    m_highlightingRules.clear();
    QString lower = ext.toLower();
    if (lower == "py") {
        setupPythonRules();
    } else if (lower == "qml" || lower == "js" || lower == "ts") {
        setupQmlRules();
    } else {
        setupCppRules();
    }
    rehighlight();
}

void CodeSyntaxHighlighter::setupCppRules() {
    HighlightingRule rule;

    // Keyword Format
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#569cd6")); // VS Code blue
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral("\\bchar\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\bdouble\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bexplicit\\b"),
        QStringLiteral("\\bfriend\\b"), QStringLiteral("\\binline\\b"), QStringLiteral("\\bint\\b"),
        QStringLiteral("\\blong\\b"), QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\boperator\\b"),
        QStringLiteral("\\bprivate\\b"), QStringLiteral("\\bprotected\\b"), QStringLiteral("\\bpublic\\b"),
        QStringLiteral("\\bshort\\b"), QStringLiteral("\\bsignals\\b"), QStringLiteral("\\bsigned\\b"),
        QStringLiteral("\\bslots\\b"), QStringLiteral("\\bstatic\\b"), QStringLiteral("\\bstruct\\b"),
        QStringLiteral("\\btemplate\\b"), QStringLiteral("\\btypedef\\b"), QStringLiteral("\\btypename\\b"),
        QStringLiteral("\\bunion\\b"), QStringLiteral("\\bunsigned\\b"), QStringLiteral("\\bvirtual\\b"),
        QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bbool\\b"),
        QStringLiteral("\\bauto\\b"), QStringLiteral("\\boverride\\b"), QStringLiteral("\\bfinal\\b"),
        QStringLiteral("\\breturn\\b"), QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"),
        QStringLiteral("\\bfor\\b"), QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bdo\\b"),
        QStringLiteral("\\bswitch\\b"), QStringLiteral("\\bcase\\b"), QStringLiteral("\\bdefault\\b")
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        m_highlightingRules.append(rule);
    }

    // Class/Type format
    QTextCharFormat classFormat;
    classFormat.setForeground(QColor("#4ec9b0")); // Teal
    rule.pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z0-9_]+\\b"));
    rule.format = classFormat;
    m_highlightingRules.append(rule);

    // Preprocessor (#include, #define...)
    QTextCharFormat preprocFormat;
    preprocFormat.setForeground(QColor("#c586c0")); // Purple
    rule.pattern = QRegularExpression(QStringLiteral("#\\s*[a-zA-Z_]+"));
    rule.format = preprocFormat;
    m_highlightingRules.append(rule);

    // Strings
    QTextCharFormat quotationFormat;
    quotationFormat.setForeground(QColor("#ce9178")); // Orange/brown
    rule.pattern = QRegularExpression(QStringLiteral("\".*?\""));
    rule.format = quotationFormat;
    m_highlightingRules.append(rule);

    // Single line comments
    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(QColor("#6a9955")); // Green
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = singleLineCommentFormat;
    m_highlightingRules.append(rule);

    // Multi-line comments
    m_multiLineCommentFormat.setForeground(QColor("#6a9955"));
    m_commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeSyntaxHighlighter::setupPythonRules() {
    HighlightingRule rule;

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#c586c0"));
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral("\\bdef\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\bimport\\b"),
        QStringLiteral("\\bfrom\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\bif\\b"),
        QStringLiteral("\\belif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bfor\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\btry\\b"), QStringLiteral("\\bexcept\\b"),
        QStringLiteral("\\bfinally\\b"), QStringLiteral("\\bwith\\b"), QStringLiteral("\\bas\\b"),
        QStringLiteral("\\bpass\\b"), QStringLiteral("\\byield\\b"), QStringLiteral("\\blambda\\b")
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor("#ce9178"));
    rule.pattern = QRegularExpression(QStringLiteral("(\".*?\"|'.*?')"));
    rule.format = stringFormat;
    m_highlightingRules.append(rule);

    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor("#6a9955"));
    rule.pattern = QRegularExpression(QStringLiteral("#[^\n]*"));
    rule.format = commentFormat;
    m_highlightingRules.append(rule);
}

void CodeSyntaxHighlighter::setupQmlRules() {
    setupCppRules(); // Similar keywords, add component formats
    HighlightingRule rule;
    QTextCharFormat compFormat;
    compFormat.setForeground(QColor("#4ec9b0"));
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Z][A-Za-z0-9_]+\\b"));
    rule.format = compFormat;
    m_highlightingRules.append(rule);

    QTextCharFormat propFormat;
    propFormat.setForeground(QColor("#9cdcfe"));
    rule.pattern = QRegularExpression(QStringLiteral("\\b(property|signal|function|alias)\\b"));
    rule.format = propFormat;
    m_highlightingRules.append(rule);
}

void CodeSyntaxHighlighter::highlightBlock(const QString &text) {
    for (const HighlightingRule &rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    if (m_commentStartExpression.pattern().isEmpty()) return;

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(m_commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = m_commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartExpression, startIndex + commentLength);
    }
}

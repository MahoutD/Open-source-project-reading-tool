#pragma once
#include <QSyntaxHighlighter>
#include <QRegularExpression>

class CodeSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit CodeSyntaxHighlighter(QTextDocument *parent = nullptr);

    void setFileType(const QString &ext);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> m_highlightingRules;

    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;
    QTextCharFormat m_multiLineCommentFormat;

    void setupCppRules();
    void setupPythonRules();
    void setupQmlRules();
};

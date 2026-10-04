#pragma once
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QString>
#include "CodeSyntaxHighlighter.h"

class CodeEditorBridge : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(QString fileExtension READ fileExtension WRITE setFileExtension NOTIFY fileExtensionChanged)

public:
    explicit CodeEditorBridge(QQuickItem *parent = nullptr);

    QQuickTextDocument* textDocument() const { return m_textDoc; }
    void setTextDocument(QQuickTextDocument* doc);

    QString fileExtension() const { return m_fileExtension; }
    void setFileExtension(const QString &ext);

signals:
    void textDocumentChanged();
    void fileExtensionChanged();

private:
    QQuickTextDocument *m_textDoc = nullptr;
    QString m_fileExtension;
    CodeSyntaxHighlighter *m_highlighter = nullptr;
};

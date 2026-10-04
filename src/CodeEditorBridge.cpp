#include "CodeEditorBridge.h"

CodeEditorBridge::CodeEditorBridge(QQuickItem *parent)
    : QQuickItem(parent)
{
}

void CodeEditorBridge::setTextDocument(QQuickTextDocument *doc) {
    if (m_textDoc == doc) return;
    m_textDoc = doc;

    if (m_highlighter) {
        delete m_highlighter;
        m_highlighter = nullptr;
    }

    if (m_textDoc && m_textDoc->textDocument()) {
        m_highlighter = new CodeSyntaxHighlighter(m_textDoc->textDocument());
        m_highlighter->setFileType(m_fileExtension);
    }

    emit textDocumentChanged();
}

void CodeEditorBridge::setFileExtension(const QString &ext) {
    if (m_fileExtension == ext) return;
    m_fileExtension = ext;
    if (m_highlighter) {
        m_highlighter->setFileType(ext);
    }
    emit fileExtensionChanged();
}

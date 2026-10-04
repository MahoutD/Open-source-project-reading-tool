#include "Logger.h"
#include <QDebug>

Logger::Logger(QObject *parent) : QObject(parent) {}

Logger* Logger::instance() {
    static Logger s_instance;
    return &s_instance;
}

void Logger::appendLog(const QString &level, const QString &category, const QString &message) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QVariantMap item;
    item["level"] = level;
    item["category"] = category;
    item["message"] = message;
    item["timestamp"] = timeStr;

    m_logs.append(item);
    if (m_logs.size() > 500) {
        m_logs.removeFirst();
    }
    emit logsChanged();
    emit newLogAdded(level, category, message, timeStr);

    qDebug().noquote() << QString("[%1] [%2] [%3] %4").arg(timeStr, level, category, message);
}

void Logger::logInfo(const QString &category, const QString &message) {
    appendLog("INFO", category, message);
}

void Logger::logWarning(const QString &category, const QString &message) {
    appendLog("WARN", category, message);
}

void Logger::logError(const QString &category, const QString &message) {
    appendLog("ERROR", category, message);
}

void Logger::logSuccess(const QString &category, const QString &message) {
    appendLog("SUCCESS", category, message);
}

void Logger::clear() {
    m_logs.clear();
    emit logsChanged();
}

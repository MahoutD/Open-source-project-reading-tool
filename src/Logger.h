#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>

class Logger : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(int count READ count NOTIFY logsChanged)

public:
    static Logger* instance();

    QVariantList logs() const { return m_logs; }
    int count() const { return m_logs.size(); }

    Q_INVOKABLE void logInfo(const QString &category, const QString &message);
    Q_INVOKABLE void logWarning(const QString &category, const QString &message);
    Q_INVOKABLE void logError(const QString &category, const QString &message);
    Q_INVOKABLE void logSuccess(const QString &category, const QString &message);
    Q_INVOKABLE void clear();

signals:
    void logsChanged();
    void newLogAdded(const QString &level, const QString &category, const QString &message, const QString &timestamp);

private:
    explicit Logger(QObject *parent = nullptr);
    void appendLog(const QString &level, const QString &category, const QString &message);

    QVariantList m_logs;
};

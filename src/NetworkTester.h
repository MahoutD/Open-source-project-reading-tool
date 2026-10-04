#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>

class NetworkTester : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool testing READ isTesting NOTIFY testingChanged)

public:
    explicit NetworkTester(QObject *parent = nullptr);

    bool isTesting() const { return m_testing; }

    Q_INVOKABLE void testPlatform(const QString &platformKey, const QString &url);
    Q_INVOKABLE void testAllPlatforms();

signals:
    void testingChanged();
    void testResultReady(const QString &platformKey, bool success, int httpCode, qint64 latencyMs, const QString &message);

private:
    QNetworkAccessManager *m_nam;
    bool m_testing = false;
    int m_pendingCount = 0;
};

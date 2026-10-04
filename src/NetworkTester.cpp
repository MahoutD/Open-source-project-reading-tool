#include "NetworkTester.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QUrl>

NetworkTester::NetworkTester(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {}

void NetworkTester::testPlatform(const QString &platformKey, const QString &url) {
    if (!m_testing) {
        m_testing = true;
        emit testingChanged();
    }
    m_pendingCount++;

    QUrl targetUrl(url);
    QNetworkRequest request(targetUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) CodeReader/1.0");
    request.setTransferTimeout(5000); // 5s timeout

    QElapsedTimer *timer = new QElapsedTimer();
    timer->start();

    QNetworkReply *reply = m_nam->head(request); // Try HEAD request first for speed
    
    auto handleFinish = [this, reply, timer, platformKey, url]() {
        qint64 latency = timer->elapsed();
        delete timer;

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QNetworkReply::NetworkError err = reply->error();
        bool success = (err == QNetworkReply::NoError) || (statusCode >= 200 && statusCode < 400);

        QString msg;
        if (success) {
            msg = QString("连接成功 (HTTP %1, 耗时 %2 ms)").arg(statusCode).arg(latency);
        } else {
            if (err == QNetworkReply::TimeoutError) {
                msg = "连接超时 (超过 5000ms)";
            } else if (statusCode > 0) {
                msg = QString("HTTP 返回 %1").arg(statusCode);
                // 403 or 401 still means network reached
                if (statusCode == 401 || statusCode == 403 || statusCode == 405) {
                    success = true;
                    msg += " (服务器可正常响应)";
                }
            } else {
                msg = reply->errorString();
            }
        }

        emit testResultReady(platformKey, success, statusCode, latency, msg);

        reply->deleteLater();
        m_pendingCount--;
        if (m_pendingCount <= 0) {
            m_pendingCount = 0;
            m_testing = false;
            emit testingChanged();
        }
    };

    connect(reply, &QNetworkReply::finished, this, handleFinish);
}

void NetworkTester::testAllPlatforms() {
    testPlatform("github", "https://github.com");
    testPlatform("gitee", "https://gitee.com");
    testPlatform("gitlab", "https://gitlab.com");
    testPlatform("gitcode", "https://gitcode.com");
}

#include "NetworkTester.h"
#include "Logger.h"
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

    Logger::instance()->logInfo("网络探测", QString("发起连接测试 -> %1 (%2)").arg(platformKey, url));

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
            Logger::instance()->logSuccess("网络探测", QString("%1 探测成功: %2").arg(platformKey, msg));
        } else {
            if (err == QNetworkReply::TimeoutError) {
                msg = "连接超时 (超过 5000ms)";
                Logger::instance()->logError("网络探测", QString("%1 超时，请检查代理网络配置").arg(platformKey));
            } else if (statusCode > 0) {
                msg = QString("HTTP 返回 %1").arg(statusCode);
                if (statusCode == 401 || statusCode == 403 || statusCode == 405) {
                    success = true;
                    msg += " (服务器可正常响应)";
                    Logger::instance()->logInfo("网络探测", QString("%1: %2").arg(platformKey, msg));
                } else {
                    Logger::instance()->logWarning("网络探测", QString("%1 返回非200状态: %2").arg(platformKey, msg));
                }
            } else {
                msg = reply->errorString();
                Logger::instance()->logError("网络探测", QString("%1 连接异常: %2").arg(platformKey, msg));
            }
        }

        emit testResultReady(platformKey, success, statusCode, latency, msg);

        reply->deleteLater();
        m_pendingCount--;
        if (m_pendingCount <= 0) {
            m_pendingCount = 0;
            m_testing = false;
            emit testingChanged();
            Logger::instance()->logInfo("网络探测", "所有平台连通性测试已完成");
        }
    };

    connect(reply, &QNetworkReply::finished, this, handleFinish);
}

void NetworkTester::testAllPlatforms() {
    Logger::instance()->logInfo("网络探测", "开始执行全平台连通性批量测试...");
    testPlatform("github", "https://github.com");
    testPlatform("gitee", "https://gitee.com");
    testPlatform("gitlab", "https://gitlab.com");
    testPlatform("gitcode", "https://gitcode.com");
}

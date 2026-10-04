#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QDir>
#include "NetworkTester.h"
#include "ProjectController.h"
#include "GraphVisualizer.h"
#include "InteractiveGraphView.h"
#include "CodeEditorBridge.h"
#include "Logger.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("OpenSourceCodeReader");
    app.setOrganizationName("Antigravity");

    QQuickStyle::setStyle("Basic");

    // Register QML types
    qmlRegisterType<GraphVisualizer>("CodeReader", 1, 0, "GraphVisualizer");
    qmlRegisterType<InteractiveGraphView>("CodeReader", 1, 0, "InteractiveGraphView");
    qmlRegisterType<CodeEditorBridge>("CodeReader", 1, 0, "CodeEditorBridge");

    QQmlApplicationEngine engine;

    NetworkTester networkTester;
    ProjectController projectController;
    Logger *logger = Logger::instance();

    engine.rootContext()->setContextProperty("networkTester", &networkTester);
    engine.rootContext()->setContextProperty("projectController", &projectController);
    engine.rootContext()->setContextProperty("appLogger", logger);

    logger->logInfo("系统核心", "开源项目代码阅读工具启动初始化...");
    logger->logSuccess("系统核心", "C++ MSVC / Qt 6.8.3 QML 渲染环境就绪");

    // Initial working directory
    projectController.openLocalFolder(QDir::currentPath());

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}

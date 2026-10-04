#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QDir>
#include "NetworkTester.h"
#include "ProjectController.h"
#include "GraphVisualizer.h"
#include "CodeEditorBridge.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("OpenSourceCodeReader");
    app.setOrganizationName("Antigravity");

    QQuickStyle::setStyle("Basic");

    // Register QML types
    qmlRegisterType<GraphVisualizer>("CodeReader", 1, 0, "GraphVisualizer");
    qmlRegisterType<CodeEditorBridge>("CodeReader", 1, 0, "CodeEditorBridge");

    QQmlApplicationEngine engine;

    NetworkTester networkTester;
    ProjectController projectController;

    engine.rootContext()->setContextProperty("networkTester", &networkTester);
    engine.rootContext()->setContextProperty("projectController", &projectController);

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

#include "video.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    registerVideo();
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("apiOrigin", QStringLiteral("https://api.bilibili.com"));
    engine.load(QUrl("qrc:/Main.qml"));
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}

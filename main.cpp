#include "video.h"
#include "account.h"
#include <QNetworkAccessManager>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    registerVideo();
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("apiOrigin", QStringLiteral("https://api.bilibili.com"));
    auto *account = new Account;
    engine.networkAccessManager()->setCookieJar(account);
    engine.rootContext()->setContextProperty("account", account);
    engine.rootContext()->setContextProperty("passportOrigin", QStringLiteral("https://passport.bilibili.com"));
    engine.load(QUrl("qrc:/Main.qml"));
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QInputMethodEvent>
#include <QQuickWindow>
#include <QtTest>
#include <QProcess>
#include <QtEndian>
#include <cmath>
#include "video.h"
#include "account.h"
#include <QNetworkAccessManager>

class FixtureAccount : public Account {
public:
    bool cleared = false;
    bool existing = true;
    bool saved = false;
    void save() override { saved = true; }
    void restore() override { emit restored(existing); }
    bool deletionWorks = true;
    void clearSession() override {
        setAllCookies({});
        cleared = true;
        if (deletionWorks) existing = false;
        emit sessionCleared(deletionWorks);
    }
};

class PlaybackTest : public QObject {
    Q_OBJECT
private slots:
    void signsInWithQr() {
        QQmlApplicationEngine engine;
        auto *account = new FixtureAccount;
        account->existing = false;
        engine.networkAccessManager()->setCookieJar(account);
        engine.rootContext()->setContextProperty("account", account);
        const auto origin = qEnvironmentVariable("MISAKA_API") + "/qr";
        engine.rootContext()->setContextProperty("apiOrigin", origin);
        engine.rootContext()->setContextProperty("passportOrigin", origin);
        engine.load(QUrl("qrc:/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto *action = window->findChild<QQuickItem *>("accountAction");
        auto *panel = window->findChild<QQuickItem *>("accountPanel");
        QVERIFY(action && panel);
        QTest::mouseClick(window, Qt::LeftButton, {}, action->mapToScene(action->boundingRect().center()).toPoint());
        QTRY_VERIFY(panel->property("qr").toString().startsWith("data:image/png"));
        QTRY_COMPARE(panel->property("identity").toString(), QStringLiteral("已登录 · 测试账号"));
        auto *avatar = window->findChild<QQuickItem *>("accountAvatar");
        QVERIFY(avatar && avatar->isVisible());
        QTRY_COMPARE(avatar->property("status").toInt(), 1); // Image.Ready
        QVERIFY(account->saved);
        auto *logout = window->findChild<QQuickItem *>("logoutAction");
        QVERIFY(logout && logout->isVisible());
        account->deletionWorks = false;
        QVERIFY(QMetaObject::invokeMethod(logout, "click"));
        QCOMPARE(panel->property("identity").toString(), QStringLiteral("未登录"));
        QVERIFY(!avatar->isVisible());
        QVERIFY(account->cookiesForUrl(QUrl(origin)).isEmpty());
        auto *retry = window->findChild<QQuickItem *>("retryDelete");
        QVERIFY(retry && retry->isVisible());
        account->deletionWorks = true;
        QVERIFY(QMetaObject::invokeMethod(retry, "click"));
        QTRY_VERIFY(action->isEnabled());
        QVERIFY(!retry->isVisible());
    }
    void logoutRestoredAccount_data() {
        QTest::addColumn<bool>("delayed");
        QTest::newRow("avatar unavailable") << false;
        QTest::newRow("validation in flight") << true;
    }
    void logoutRestoredAccount() {
        QFETCH(bool, delayed);
        FixtureAccount account;
        auto open = [&] {
            auto engine = std::make_unique<QQmlApplicationEngine>();
            engine->rootContext()->setContextProperty("account", &account);
            engine->rootContext()->setContextProperty("apiOrigin", qEnvironmentVariable("MISAKA_API")
                + (delayed ? "/delayed" : "/restore"));
            engine->load(QUrl("qrc:/Main.qml"));
            return engine;
        };
        auto engine = open();
        auto *window = qobject_cast<QQuickWindow *>(engine->rootObjects().value(0));
        QVERIFY(window && QTest::qWaitForWindowExposed(window));
        if (!delayed) {
            auto *fallback = window->findChild<QQuickItem *>("avatarFallback");
            QVERIFY(fallback);
            QTRY_VERIFY(fallback->isVisible());
            QCOMPARE(fallback->property("text").toString(), QStringLiteral("测"));
        }
        auto *logout = window->findChild<QQuickItem *>("logoutAction");
        QVERIFY(logout && logout->isVisible());
        QTest::mouseClick(window, Qt::LeftButton, {}, logout->mapToScene(logout->boundingRect().center()).toPoint());
        if (delayed) QTest::qWait(650);
        QVERIFY(account.cleared && !logout->isVisible());
        engine.reset();
        engine = open();
        auto *panel = engine->rootObjects().first()->findChild<QQuickItem *>("accountPanel");
        QVERIFY(panel);
        QCOMPARE(panel->property("identity").toString(), QStringLiteral("未登录"));
    }
    void restoredSessionRefusedThenExpired() {
        QQmlApplicationEngine engine;
        auto *account = new FixtureAccount;
        engine.networkAccessManager()->setCookieJar(account);
        engine.rootContext()->setContextProperty("account", account);
        engine.rootContext()->setContextProperty("apiOrigin", qEnvironmentVariable("MISAKA_API") + "/account");
        engine.load(QUrl("qrc:/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto *panel = window->findChild<QQuickItem *>("accountPanel");
        auto *action = window->findChild<QQuickItem *>("accountAction");
        QVERIFY(panel && action);
        QTRY_COMPARE(panel->property("notice").toString(), QStringLiteral("服务拒绝请求 (412)"));
        QCOMPARE(panel->property("identity").toString(), QStringLiteral("会话待验证"));
        QVERIFY(!account->cleared);
        QTest::mouseClick(window, Qt::LeftButton, {}, action->mapToScene(action->boundingRect().center()).toPoint());
        QTRY_COMPARE(action->property("text").toString(), QStringLiteral("重新登录"));
        QVERIFY(account->cleared);
    }
    void opensP1AndSeeks() {
        QTest::failOnWarning(QRegularExpression(QStringLiteral(".*Cannot open file.*icons/.*[.]svg.*")));
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("apiOrigin", qEnvironmentVariable("MISAKA_API"));
        engine.load(QUrl("qrc:/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto item = [&](const char *name) { return window->findChild<QQuickItem *>(name); };
        auto click = [&](QQuickItem *target, double fraction) {
            auto point = target->mapToScene({target->width()*fraction, target->height()/2});
            QTest::mouseClick(window, Qt::LeftButton, {}, point.toPoint());
        };
        QVERIFY(item("bv") && item("timeline") && item("video"));
        QVERIFY(item("clock"));
        click(item("bv"), 0.5);
        QInputMethodEvent input; input.setCommitString("BV17x411w7KC"); QCoreApplication::sendEvent(window, &input);
        QTest::keyClick(window, Qt::Key_Return);
        if (qEnvironmentVariableIsSet("MISAKA_LIVE_SMOKE")) {
            auto *clock = item("clock");
            QVERIFY(clock);
            QElapsedTimer wait;
            wait.start();
            while (wait.elapsed() < 20000 && clock->property("text").toString().startsWith("00:00:00 /"))
                QTest::qWait(50);
            qInfo() << window->property("status") << clock->property("text");
            QVERIFY(window->property("status").toString().startsWith("P1 ·"));
            QVERIFY(!clock->property("text").toString().startsWith("00:00:00 /"));
            return;
        }
        auto centerColor = [&] {
            const QImage frame = window->grabWindow();
            return frame.pixelColor(frame.width()/2, frame.height()/2);
        };
        QTRY_VERIFY_WITH_TIMEOUT(centerColor().red() > 200 && centerColor().blue() < 60, 8000);
        QTRY_VERIFY(item("timeline")->isEnabled());
        QVERIFY(item("clock")->property("text").toString().startsWith("00:00:00 / 00:00:10"));
        auto *toggle = item("playPause");
        QVERIFY(toggle);
        click(toggle, 0.5);
        QTRY_COMPARE(toggle->property("text").toString(), QStringLiteral("继续播放"));
        auto *mute = item("mute");
        auto *volume = item("volume");
        QVERIFY(mute && volume);
        click(volume, 0.55);
        const int level = qRound(volume->property("value").toDouble());
        QVERIFY(level > 45 && level < 65);
        click(mute, 0.5);
        QTRY_COMPARE(mute->property("text").toString(), QStringLiteral("取消静音"));
        click(mute, 0.5);
        QTRY_COMPARE(mute->property("text").toString(), QStringLiteral("静音"));
        QCOMPARE(qRound(volume->property("value").toDouble()), level);
        click(volume, 0.98);
        click(item("timeline"), 0.33);
        auto clockText = [&] { return item("clock")->property("text").toString(); };
        QTRY_VERIFY(clockText().startsWith("00:00:03 /"));
        QTest::qWait(1200);
        QVERIFY(clockText().startsWith("00:00:03 /"));
        click(toggle, 0.5);
        QTRY_COMPARE(toggle->property("text").toString(), QStringLiteral("暂停"));
        QTRY_VERIFY(clockText().startsWith("00:00:04 /"));
        QProcess monitor;
        const QString device = qEnvironmentVariable("MISAKA_TEST_MONITOR");
        if (!device.isEmpty()) {
            monitor.start("parec", {"--device", device, "--raw", "--format=s16le",
                                    "--rate=16000", "--channels=1", "--latency-msec=10"});
            QVERIFY(monitor.waitForStarted());
        }
        click(item("timeline"), 0.72);
        QTRY_VERIFY_WITH_TIMEOUT(centerColor().blue() > 200 && centerColor().red() < 60, 2000);
        if (device.isEmpty()) return;

        // The fixture flashes a white square and sounds a tone for the
        // first 150 ms of each second. Observe both outputs after the seek.
        monitor.readAllStandardOutput();
        QElapsedTimer elapsed;
        elapsed.start();
        QList<qint64> flashes, tones;
        bool lastFlash = false, lastTone = false;
        while (elapsed.elapsed() < 2300) {
            QTest::qWait(10);
            const QImage frame = window->grabWindow().convertToFormat(QImage::Format_RGB32);
            bool flash = false;
            for (int y = frame.height()/5; y < frame.height()*4/5 && !flash; y += 12)
                for (int x = frame.width()/8; x < frame.width()*7/8; x += 12) {
                    const QColor pixel(frame.pixel(x, y));
                    if (pixel.red() > 220 && pixel.green() > 220 && pixel.blue() > 220) {
                        flash = true;
                        break;
                    }
                }
            if (flash && !lastFlash) flashes.append(elapsed.elapsed());
            lastFlash = flash;
            const QByteArray samples = monitor.readAllStandardOutput();
            if (samples.isEmpty()) continue;
            bool tone = false;
            for (int i = 0; i + 1 < samples.size(); i += 2)
                if (std::abs(int(qFromLittleEndian<qint16>(samples.constData()+i))) > 200)
                    tone = true;
            if (tone && !lastTone) tones.append(elapsed.elapsed());
            lastTone = tone;
        }
        monitor.terminate();
        monitor.waitForFinished();
        qInfo() << "Post-seek flash / tone onsets (ms)" << flashes << tones;
        QVERIFY(flashes.size() >= 2);
        for (auto flash : flashes) {
            QVERIFY(std::any_of(tones.begin(), tones.end(), [flash](auto tone) {
                return std::abs(flash - tone) < 120;
            }));
        }
    }
};
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    registerVideo();
    PlaybackTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "playback.moc"

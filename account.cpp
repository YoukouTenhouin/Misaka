#include "account.h"
#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <qt6keychain/keychain.h>

// Public libqrencode ABI, also available on runtime-only installations.
extern "C" {
struct QRcode { int version; int width; unsigned char *data; };
QRcode *QRcode_encodeString8bit(const char *, int, int);
void QRcode_free(QRcode *);
}

QString Account::qrImage(const QString &url) {
    auto *code = QRcode_encodeString8bit(url.toUtf8().constData(), 0, 0);
    if (!code) return {};
    QImage image(code->width + 8, code->width + 8, QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int y = 0; y < code->width; ++y)
        for (int x = 0; x < code->width; ++x)
            if (code->data[y * code->width + x] & 1)
                image.setPixelColor(x + 4, y + 4, Qt::black);
    QRcode_free(code);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return "data:image/png;base64," + QString::fromLatin1(png.toBase64());
}

void Account::restore() {
    auto *job = new QKeychain::ReadPasswordJob("Misaka", this);
    job->setKey("session");
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        QList<QNetworkCookie> cookies;
        if (!job->error())
            for (const auto &line : job->binaryData().split('\n'))
                cookies.append(QNetworkCookie::parseCookies(line));
        setAllCookies(cookies);
        emit restored(!cookies.isEmpty());
    });
    job->start();
}

void Account::save() {
    QByteArray data;
    for (const auto &cookie : cookiesForUrl(QUrl("https://api.bilibili.com/")))
        data += cookie.toRawForm() + '\n';
    auto *job = new QKeychain::WritePasswordJob("Misaka", this);
    job->setKey("session");
    job->setBinaryData(data);
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        if (job->error()) emit storageError("无法保存会话，下次启动需要重新扫码");
    });
    job->start();
}

void Account::clearSession() {
    setAllCookies({});
    auto *job = new QKeychain::DeletePasswordJob("Misaka", this);
    job->setKey("session");
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        emit sessionCleared(!job->error() || job->error() == QKeychain::EntryNotFound);
    });
    job->start();
}

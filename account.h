#pragma once
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QObject>

class Account : public QNetworkCookieJar {
    Q_OBJECT
public:
    using QNetworkCookieJar::QNetworkCookieJar;
    Q_INVOKABLE QString qrImage(const QString &url);
    Q_INVOKABLE virtual void restore();
    Q_INVOKABLE virtual void save();
    Q_INVOKABLE virtual void clearSession();
signals:
    void sessionCleared(bool success);
    void restored(bool hasCookies);
    void storageError(QString message);
};

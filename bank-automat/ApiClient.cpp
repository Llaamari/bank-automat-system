#include "ApiClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent),
      m_baseUrl("http://localhost:3000")
{
}

void ApiClient::setBaseUrl(const QString &baseUrl)
{
    m_baseUrl = baseUrl.trimmed();
}

QString ApiClient::baseUrl() const
{
    return m_baseUrl;
}

QString ApiClient::joinUrl(const QString &baseUrl, const QString &path)
{
    QString b = baseUrl;
    QString p = path;

    if (b.endsWith('/')) b.chop(1);
    if (!p.startsWith('/')) p.prepend('/');

    return b + p;
}

QString ApiClient::extractErrorMessage(const QJsonDocument &json, const QString &fallback)
{
    if (json.isObject()) {
        const auto obj = json.object();
        if (obj.contains("error") && obj.value("error").isString()) {
            return obj.value("error").toString();
        }
        if (obj.contains("message") && obj.value("message").isString()) {
            return obj.value("message").toString();
        }
    }
    return fallback;
}

void ApiClient::postJson(const QString &path,
                         const QJsonObject &body,
                         std::function<void(bool, int, QJsonDocument, QString)> cb)
{
    const QUrl url(joinUrl(m_baseUrl, path));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = m_net.post(req, payload);

    QObject::connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();

        QJsonParseError parseErr;
        const QJsonDocument json = QJsonDocument::fromJson(raw, &parseErr);

        // Network-level error (connection refused, DNS, etc.)
        if (reply->error() != QNetworkReply::NoError) {
            const QString err = reply->errorString();
            reply->deleteLater();
            cb(false, status, json, err);
            return;
        }

        // HTTP error (400/401/500). Backend often returns JSON with {error:"..."} or {ok:false}
        if (status < 200 || status >= 300) {
            const QString err = (parseErr.error == QJsonParseError::NoError)
                                    ? ApiClient::extractErrorMessage(json, QString("HTTP %1").arg(status))
                                    : QString("HTTP %1").arg(status);
            reply->deleteLater();
            cb(false, status, json, err);
            return;
        }

        reply->deleteLater();
        cb(true, status, json, QString());
    });
}

void ApiClient::getJson(const QString &path,
                        std::function<void(bool, int, QJsonDocument, QString)> cb)
{
    const QUrl url(joinUrl(m_baseUrl, path));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_net.get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();

        QJsonParseError parseErr;
        const QJsonDocument json = QJsonDocument::fromJson(raw, &parseErr);

        if (reply->error() != QNetworkReply::NoError) {
            const QString err = reply->errorString();
            reply->deleteLater();
            cb(false, status, json, err);
            return;
        }

        if (status < 200 || status >= 300) {
            const QString err = (parseErr.error == QJsonParseError::NoError)
                                    ? ApiClient::extractErrorMessage(json, QString("HTTP %1").arg(status))
                                    : QString("HTTP %1").arg(status);
            reply->deleteLater();
            cb(false, status, json, err);
            return;
        }

        reply->deleteLater();
        cb(true, status, json, QString());
    });
}

// -------- Public API methods --------

void ApiClient::login(const QString &cardNumber, const QString &pin)
{
    QJsonObject body;
    body["cardNumber"] = cardNumber.trimmed();
    body["pin"] = pin;

    postJson("/auth/login", body,
    [this](bool ok, int httpStatus, QJsonDocument json, QString error)
{
    // Jos HTTP 401 → väärä login
    if (!ok && httpStatus == 401) {
        emit loginResult(false, -1, "Wrong card number or PIN");
        return;
    }

    if (!ok) {
        emit loginResult(false, -1,
            error.isEmpty() ? "Login failed." : error);
        return;
    }

    if (!json.isObject()) {
        emit loginResult(false, -1,
            "Invalid response from server");
        return;
    }

    const QJsonObject obj = json.object();
    const bool loginOk = obj.value("ok").toBool(false);
    const int accountId = obj.value("accountId").toInt(-1);

    if (!loginOk || accountId <= 0) {
        emit loginResult(false, -1,
            "Wrong card number or PIN");
        return;
    }

    emit loginResult(true, accountId, QString());
});
}

void ApiClient::getBalance(int accountId)
{
    getJson(QString("/accounts/%1/balance").arg(accountId),
            [this](bool ok, int /*status*/, QJsonDocument json, QString error) {
        if (!ok) {
            emit balanceResult(false, QJsonObject(), error.isEmpty() ? "Failed to load balance" : error);
            return;
        }
        if (!json.isObject()) {
            emit balanceResult(false, QJsonObject(), "Invalid response from server");
            return;
        }
        emit balanceResult(true, json.object(), QString());
    });
}

void ApiClient::withdraw(int accountId, int amount)
{
    QJsonObject body;
    body["amount"] = amount;

    postJson(QString("/accounts/%1/withdraw").arg(accountId), body,
             [this](bool ok, int /*status*/, QJsonDocument json, QString error) {
        if (!ok) {
            emit withdrawResult(false, QJsonObject(), error.isEmpty() ? "Withdraw failed" : error);
            return;
        }
        if (!json.isObject()) {
            emit withdrawResult(false, QJsonObject(), "Invalid response from server");
            return;
        }
        emit withdrawResult(true, json.object(), QString());
    });
}

void ApiClient::getTransactions(int accountId, int limit)
{
    // Backend supports ?limit=10
    const int safeLimit = (limit <= 0) ? 10 : (limit > 100 ? 100 : limit);

    getJson(QString("/accounts/%1/transactions?limit=%2").arg(accountId).arg(safeLimit),
            [this](bool ok, int /*status*/, QJsonDocument json, QString error) {
        if (!ok) {
            emit transactionsResult(false, QJsonArray(), error.isEmpty() ? "Failed to load transactions" : error);
            return;
        }
        if (!json.isArray()) {
            emit transactionsResult(false, QJsonArray(), "Invalid response from server");
            return;
        }
        emit transactionsResult(true, json.array(), QString());
    });
}
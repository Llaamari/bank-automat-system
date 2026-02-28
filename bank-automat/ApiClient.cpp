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

void ApiClient::fetchImageByFilename(const QString& filename,
                                    std::function<void(const QByteArray& data)> onSuccess,
                                    std::function<void(const QString& error)> onError)
{
    const QString fn = filename.trimmed();
    if (fn.isEmpty()) {
        onError(QStringLiteral("No filename"));
        return;
    }

    const QUrl url(joinUrl(m_baseUrl, "/images/uploads/" + fn));
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "image/*");

    QNetworkReply* reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString err = reply->errorString();
            reply->deleteLater();
            onError(err);
            return;
        }
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        onSuccess(data);
    });
}

void ApiClient::getCustomerImageFilenameForAccount(
    int accountId,
    std::function<void(bool ok, const QString& filename, const QString& error)> cb)
{
    // /crud/accounts/:id -> customer_id
    const QString accountPath = QString("/crud/accounts/%1").arg(accountId);
    getJson(accountPath, [this, cb](bool ok, int httpStatus, QJsonDocument json, QString error) {
        if (!ok) {
            const QString msg = error.isEmpty()
                ? QString("Failed to fetch account (HTTP %1)").arg(httpStatus)
                : error;
            cb(false, QString(), msg);
            return;
        }
        if (!json.isObject()) {
            cb(false, QString(), QStringLiteral("Invalid account response"));
            return;
        }

        const QJsonObject acc = json.object();

        // Common field names: customer_id or customerId
        int customerId = -1;
        if (acc.contains("customer_id")) customerId = acc.value("customer_id").toInt(-1);
        if (customerId < 0 && acc.contains("customerId")) customerId = acc.value("customerId").toInt(-1);

        if (customerId < 0) {
            cb(false, QString(), QStringLiteral("Account does not contain customer_id"));
            return;
        }

        // /crud/customers/:id -> image_filename
        const QString custPath = QString("/crud/customers/%1").arg(customerId);
        getJson(custPath, [cb](bool ok2, int http2, QJsonDocument json2, QString error2) {
            if (!ok2) {
                const QString msg = error2.isEmpty()
                    ? QString("Failed to fetch customer (HTTP %1)").arg(http2)
                    : error2;
                cb(false, QString(), msg);
                return;
            }
            if (!json2.isObject()) {
                cb(false, QString(), QStringLiteral("Invalid customer response"));
                return;
            }

            const QJsonObject cust = json2.object();
            QString filename;

            if (cust.contains("image_filename") && !cust.value("image_filename").isNull()) {
                filename = cust.value("image_filename").toString();
            }

            cb(true, filename, QString());
        });
    });
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

        // Network-level error
        if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();

        if (parseErr.error == QJsonParseError::NoError) {
            // If backend returned { error: "..." }, show that instead of "Bad Request"
            err = ApiClient::extractErrorMessage(json, err);
        }

        reply->deleteLater();
        cb(false, status, json, err);
        return;
    }

        // HTTP error
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
        QString err = reply->errorString();
        if (parseErr.error == QJsonParseError::NoError) {
            err = ApiClient::extractErrorMessage(json, err);
        }
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

        // -------------------------
        // Error handling
        // -------------------------
        if (!ok) {
            // 401: wrong PIN (or unknown cardNumber). Backend may include attemptsLeft.
            if (httpStatus == 401 && json.isObject()) {
                const QJsonObject obj = json.object();
                if (obj.contains("attemptsLeft")) {
                    const int left = obj.value("attemptsLeft").toInt(-1);
                    if (left >= 0) {
                        const int used = 3 - left;
                        const QString msg = QString("Incorrect PIN (%1/3)").arg(used);
                        emit loginAccountsResult(false, QJsonArray(), msg);
                        emit loginResult(false, -1, msg);
                        return;
                    }
                }

                const QString msg = QStringLiteral("Incorrect card number or PIN");
                emit loginAccountsResult(false, QJsonArray(), msg);
                emit loginResult(false, -1, msg);
                return;
            }

            // 403: card locked
            if (httpStatus == 403) {
                QString msg = error.isEmpty() ? QStringLiteral("Card locked") : error;
                if (msg.contains("liian monta yritystä", Qt::CaseInsensitive)) {
                    msg = QStringLiteral("Card locked (too many attempts)");
                }
                emit loginAccountsResult(false, QJsonArray(), msg);
                emit loginResult(false, -1, msg);
                return;
            }

            const QString msg = error.isEmpty() ? QStringLiteral("Login failed") : error;
            emit loginAccountsResult(false, QJsonArray(), msg);
            emit loginResult(false, -1, msg);
            return;
        }

        // -------------------------
        // Success handling
        // -------------------------
        if (!json.isObject()) {
            const QString msg = QStringLiteral("Invalid response from server");
            emit loginAccountsResult(false, QJsonArray(), msg);
            emit loginResult(false, -1, msg);
            return;
        }

        const QJsonObject obj = json.object();
        const bool loginOk = obj.value("ok").toBool(false);

        if (!loginOk) {
            const QString msg = QStringLiteral("Incorrect card number or PIN");
            emit loginAccountsResult(false, QJsonArray(), msg);
            emit loginResult(false, -1, msg);
            return;
        }

        // New format: { ok:true, accounts:[{role, accountId}, ...] }
        if (obj.contains("accounts") && obj.value("accounts").isArray()) {
            const QJsonArray accounts = obj.value("accounts").toArray();
            if (accounts.isEmpty()) {
                const QString msg = QStringLiteral("The card has no linked accounts");
                emit loginAccountsResult(false, QJsonArray(), msg);
                emit loginResult(false, -1, msg);
                return;
            }

            emit loginAccountsResult(true, accounts, QString());

            // Backward-compatible: pick one accountId (prefer debit)
            int preferredId = -1;
            for (const auto &v : accounts) {
                const QJsonObject o = v.toObject();
                if (o.value("role").toString() == "debit") {
                    preferredId = o.value("accountId").toInt(-1);
                    break;
                }
            }
            if (preferredId <= 0) {
                preferredId = accounts.at(0).toObject().value("accountId").toInt(-1);
            }
            emit loginResult(true, preferredId, QString());
            return;
        }

        // Backward-compatible fallback: { ok:true, accountId:<int> }
        const int accountId = obj.value("accountId").toInt(-1);
        if (accountId <= 0) {
            const QString msg = QStringLiteral("Invalid response from server");
            emit loginAccountsResult(false, QJsonArray(), msg);
            emit loginResult(false, -1, msg);
            return;
        }

        QJsonArray accounts;
        QJsonObject a;
        a["role"] = "debit";
        a["accountId"] = accountId;
        accounts.append(a);

        emit loginAccountsResult(true, accounts, QString());
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
    // Backward-compatible: first page
    getTransactionsPage(accountId, limit, QString(), QString());
}

void ApiClient::getTransactionsPage(int accountId, int limit,
                                    const QString& before,
                                    const QString& after)
{
    const int safeLimit = (limit <= 0) ? 10 : (limit > 100 ? 100 : limit);

    QString path = QString("/accounts/%1/transactions?limit=%2")
                       .arg(accountId)
                       .arg(safeLimit);

    if (!before.isEmpty()) {
        path += QString("&before=%1").arg(QString(QUrl::toPercentEncoding(before)));
    }
    if (!after.isEmpty()) {
        path += QString("&after=%1").arg(QString(QUrl::toPercentEncoding(after)));
    }

    getJson(path, [this](bool ok, int /*status*/, QJsonDocument json, QString error) {
        if (!ok) {
            const QString msg = error.isEmpty() ? "Failed to load transactions" : error;
            emit transactionsPageResult(false, QJsonArray(), QString(), QString(), msg);
            emit transactionsResult(false, QJsonArray(), msg);
            return;
        }

        // Accept both old (array) and new (object) response shapes
        if (json.isArray()) {
            const QJsonArray arr = json.array();
            emit transactionsPageResult(true, arr, QString(), QString(), QString());
            emit transactionsResult(true, arr, QString());
            return;
        }

        if (!json.isObject()) {
            const QString msg = "Invalid response from server";
            emit transactionsPageResult(false, QJsonArray(), QString(), QString(), msg);
            emit transactionsResult(false, QJsonArray(), msg);
            return;
        }

        const QJsonObject obj = json.object();
        const QJsonArray items = obj.value("items").toArray();
        const QString nextCursor = obj.value("nextCursor").toString();
        const QString prevCursor = obj.value("prevCursor").toString();

        emit transactionsPageResult(true, items, nextCursor, prevCursor, QString());
        emit transactionsResult(true, items, QString());
    });
}
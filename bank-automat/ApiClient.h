#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <functional>

class ApiClient : public QObject
{
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);

    void setBaseUrl(const QString& baseUrl);
    QString baseUrl() const;

    // API calls
    void login(const QString& cardNumber, const QString& pin);
    void getBalance(int accountId);
    void withdraw(int accountId, int amount);
    void getTransactions(int accountId, int limit = 10);

signals:
    // Backward-compatible: if backend returns multiple accounts, this will pick one (prefer debit).
    void loginResult(bool ok, int accountId, QString error);
    // On success returns linked accounts: [{"role":"debit"|"credit", "accountId": <int>}]
    void loginAccountsResult(bool ok, QJsonArray accounts, QString error);
    void balanceResult(bool ok, QJsonObject data, QString error);
    void withdrawResult(bool ok, QJsonObject data, QString error);
    void transactionsResult(bool ok, QJsonArray data, QString error);

private:
    QNetworkAccessManager m_net;
    QString m_baseUrl;

    void postJson(const QString& path,
                  const QJsonObject& body,
                  std::function<void(bool ok, int httpStatus, QJsonDocument json, QString error)> cb);

    void getJson(const QString& path,
                 std::function<void(bool ok, int httpStatus, QJsonDocument json, QString error)> cb);

    static QString joinUrl(const QString& baseUrl, const QString& path);
    static QString extractErrorMessage(const QJsonDocument& json, const QString& fallback);
};
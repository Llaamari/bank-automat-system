#pragma once

#include <QDialog>
#include <QEvent>
#include <QTimer>
#include <QShowEvent>
#include <QJsonArray>
#include <QString>

class ApiClient;

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(ApiClient* api, QWidget *parent = nullptr);
    ~LoginDialog();

    int accountId() const;
    QString accountRole() const;

protected:
    // Reset inactivity timer on any user activity inside the dialog
    bool eventFilter(QObject *obj, QEvent *event) override;
    // Ensure UI is clean every time the dialog is shown
    void showEvent(QShowEvent *event) override;

private slots:
    void on_loginButton_clicked();
    void on_cancelButton_clicked();
    void onLoginResult(bool ok, QJsonArray accounts, QString error);
    void onTimeout();
    void resetTimeout();

private:
    Ui::LoginDialog *ui;
    ApiClient* m_api = nullptr;

    int m_accountId = -1;
    QString m_accountRole = "debit";

    QJsonArray m_accounts;
    bool m_waitingRoleSelection = false;

    QTimer m_timeoutTimer;
    bool m_loginInProgress = false;
};
#pragma once

#include <QDialog>
#include <QTimer>

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

private slots:
    void on_loginButton_clicked();
    void on_cancelButton_clicked();
    void onLoginResult(bool ok, int accountId, QString error);
    void onTimeout();
    void resetTimeout();

private:
    Ui::LoginDialog *ui;
    ApiClient* m_api = nullptr;
    int m_accountId = -1;
    QTimer m_timeoutTimer;
};
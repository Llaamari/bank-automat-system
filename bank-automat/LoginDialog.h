#pragma once

#include <QDialog>
#include <QEvent>
#include <QTimer>
#include <QShowEvent>

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

protected:
    // Reset inactivity timer on any user activity inside the dialog
    bool eventFilter(QObject *obj, QEvent *event) override;
    // Ensure UI is clean every time the dialog is shown
    void showEvent(QShowEvent *event) override;

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
    bool m_loginInProgress = false;
};
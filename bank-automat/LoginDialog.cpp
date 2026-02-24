#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "ApiClient.h"

#include <QMessageBox>

LoginDialog::LoginDialog(ApiClient* api, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::LoginDialog),
      m_api(api)
{
    ui->setupUi(this);

    // PIN kenttä salaiseksi
    ui->pinLineEdit->setEchoMode(QLineEdit::Password);

    // Tyhjennä error label
    ui->errorLabel->clear();
    ui->errorLabel->setStyleSheet("color: red;");

    // Yhdistä ApiClient login-signaali
    connect(m_api, &ApiClient::loginResult,
            this, &LoginDialog::onLoginResult);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

int LoginDialog::accountId() const
{
    return m_accountId;
}

void LoginDialog::on_loginButton_clicked()
{
    ui->errorLabel->clear();

    QString cardNumber = ui->cardNumberLineEdit->text().trimmed();
    QString pin = ui->pinLineEdit->text();

    if (cardNumber.isEmpty() || pin.isEmpty()) {
        ui->errorLabel->setText("Card number and PIN are required.");
        return;
    }

    // Disable button while waiting
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("Logging in...");

    m_api->login(cardNumber, pin);
}

void LoginDialog::onLoginResult(bool ok, int accountId, QString error)
{
    // Enable button again
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Login");

    if (!ok) {
        ui->errorLabel->setText(
            error.isEmpty() ? "Login failed." : error
        );
        return;
    }

    // Success
    m_accountId = accountId;

    accept();  // Sulkee dialogin QDialog::Accepted tilassa
}
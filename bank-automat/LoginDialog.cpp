#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "ApiClient.h"

#include <QEvent>
#include <QMessageBox>
#include <QWidget>

static constexpr int PIN_TIMEOUT_MS = 10 * 1000;

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

    connect(ui->pinLineEdit, &QLineEdit::returnPressed,
        this, &LoginDialog::on_loginButton_clicked);

    // --- Resetoi timeout *mistä tahansa* käyttäjäaktiivisuudesta dialogissa ---
    this->installEventFilter(this);
    const auto widgets = this->findChildren<QWidget*>();
    for (QWidget *w : widgets) {
        w->installEventFilter(this);
    }

    // --- PIN syötön aikaraja (10 s) ---
    m_timeoutTimer.setInterval(PIN_TIMEOUT_MS);
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &LoginDialog::onTimeout);

    // Resetoi timeout käyttäjäaktiivisuudesta
    connect(ui->pinLineEdit, &QLineEdit::textEdited, this, &LoginDialog::resetTimeout);
    connect(ui->cardNumberLineEdit, &QLineEdit::textEdited, this, &LoginDialog::resetTimeout);
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::resetTimeout);
    connect(ui->cancelButton, &QPushButton::clicked, this, &LoginDialog::resetTimeout);

    m_timeoutTimer.start();
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

    // Käyttäjä teki toiminnon -> resetoi aikaraja
    resetTimeout();

    QString cardNumber = ui->cardNumberLineEdit->text().trimmed();
    QString pin = ui->pinLineEdit->text();

    if (cardNumber.isEmpty() || pin.isEmpty()) {
        ui->errorLabel->setText("Card number and PIN are required.");
        return;
    }

    // Estä timeout laukeamasta kesken backend-vastauksen.
    // Jos login epäonnistuu, käynnistetään timer uudelleen onLoginResultissa.
    m_loginInProgress = true;
    m_timeoutTimer.stop();

    // Disable button while waiting
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("Logging in...");

    m_api->login(cardNumber, pin);
}

void LoginDialog::on_cancelButton_clicked()
{
    m_timeoutTimer.stop();
    reject();
}

void LoginDialog::onLoginResult(bool ok, int accountId, QString error)
{
    // Enable button again
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Login");

    m_loginInProgress = false;

    if (!ok) {
        ui->errorLabel->setText(
            error.isEmpty() ? "Login failed." : error
        );
        // Käynnistä timeout uudelleen, jotta käyttäjä ei jää dialogiin ikuisesti.
        resetTimeout();
        return;
    }

    // Success
    m_accountId = accountId;

    // Onnistunut login -> timer ei saa laueta enää
    m_timeoutTimer.stop();
    accept();  // Sulkee dialogin QDialog::Accepted tilassa
}

void LoginDialog::onTimeout()
{
    m_timeoutTimer.stop();
    m_loginInProgress = false;

    // 10 s ilman riittävää toimintaa -> takaisin aloitusnäyttöön
    ui->pinLineEdit->clear();
    ui->cardNumberLineEdit->clear();
    ui->errorLabel->clear();
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Login");

    reject();
}

void LoginDialog::resetTimeout()
{
    // Älä käynnistä timeoutia, jos odotetaan login-vastausta.
    if (m_loginInProgress) {
        return;
    }
    m_timeoutTimer.start();
}

bool LoginDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    // Kun odotetaan backend-vastausta, ei resetoida / käynnistetä timeria uudelleen.
    if (!this->isVisible()) {
        return QDialog::eventFilter(obj, event);
    }

    // Resetoi 10s laskuri kaikesta relevantista aktiviteetista.
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::Wheel:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::FocusIn:
        resetTimeout();
        break;
    default:
        break;
    }

    return QDialog::eventFilter(obj, event);
}

void LoginDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // Clean UI state every time the dialog is shown
    ui->pinLineEdit->clear();
    ui->errorLabel->clear();
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Login");

    // Focus to a sensible field
    if (ui->cardNumberLineEdit->text().trimmed().isEmpty()) {
        ui->cardNumberLineEdit->setFocus();
    } else {
        ui->pinLineEdit->setFocus();
    }

    m_loginInProgress = false;
    resetTimeout();
}
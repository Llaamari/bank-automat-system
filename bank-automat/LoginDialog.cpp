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
    // Role selection hidden until backend returns multiple linked accounts
    ui->roleLabel->setVisible(false);
    ui->roleComboBox->setVisible(false);
    ui->roleComboBox->clear();

    // PIN kenttä salaiseksi
    ui->pinLineEdit->setEchoMode(QLineEdit::Password);

    // Tyhjennä error label
    ui->errorLabel->clear();
    ui->errorLabel->setStyleSheet("color: red;");

    // Yhdistä API-kutsut
    connect(m_api, &ApiClient::loginAccountsResult,
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
QString LoginDialog::accountRole() const { return m_accountRole; }

void LoginDialog::on_loginButton_clicked()
{
    ui->errorLabel->clear();

    // If we already have multiple accounts, this click is "Continue" after choosing role
    if (m_waitingRoleSelection) {
        const QString role = ui->roleComboBox->currentData().toString();
        if (role.isEmpty()) {
            ui->errorLabel->setText("Choose account type.");
            return;
        }

        int chosenAccountId = -1;
        for (const auto &v : m_accounts) {
            const QJsonObject o = v.toObject();
            if (o.value("role").toString() == role) {
                chosenAccountId = o.value("accountId").toInt(-1);
                break;
            }
        }

        if (chosenAccountId <= 0) {
            ui->errorLabel->setText("Selected account not available.");
            return;
        }

        m_accountRole = role;
        m_accountId = chosenAccountId;
        m_timeoutTimer.stop();
        accept();
        return;
    }

    // Käyttäjä teki toiminnon -> resetoi aikaraja
    resetTimeout();

    const QString cardNumber = ui->cardNumberLineEdit->text().trimmed();
    const QString pin = ui->pinLineEdit->text();

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

void LoginDialog::onLoginResult(bool ok, QJsonArray accounts, QString error)
{
    // Enable button again
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Login");

    m_loginInProgress = false;

    if (!ok) {
        ui->errorLabel->setText(
            error.isEmpty() ? "Login failed." : error
        );
        resetTimeout();
        return;
    }

    m_accounts = accounts;
    if (m_accounts.isEmpty()) {
        ui->errorLabel->setText("No linked accounts for this card.");
        // Käynnistä timeout uudelleen, jotta käyttäjä ei jää dialogiin ikuisesti.
        resetTimeout();
        return;
    }

    // If only one option -> select automatically
    if (m_accounts.size() == 1) {
        const QJsonObject o = m_accounts.at(0).toObject();
        m_accountRole = o.value("role").toString("debit");
        m_accountId = o.value("accountId").toInt(-1);

        if (m_accountId <= 0) {
            ui->errorLabel->setText("Invalid account id from server.");
            resetTimeout();
            return;
        }

    // Onnistunut login -> timer ei saa laueta enää
    m_timeoutTimer.stop();
    accept();  // Sulkee dialogin QDialog::Accepted tilassa
        return;
    }

    // Multiple options -> show role selection
    ui->roleComboBox->clear();
    for (const auto &v : m_accounts) {
        const QJsonObject o = v.toObject();
        const QString role = o.value("role").toString();
        const int id = o.value("accountId").toInt(-1);
        if (role.isEmpty() || id <= 0) continue;
        ui->roleComboBox->addItem(QString("%1 (Account %2)").arg(role, QString::number(id)), role);
    }

    if (ui->roleComboBox->count() == 0) {
        ui->errorLabel->setText("No valid linked accounts returned.");
        resetTimeout();
        return;
    }

    ui->cardNumberLineEdit->setEnabled(false);
    ui->pinLineEdit->setEnabled(false);

    ui->roleLabel->setVisible(true);
    ui->roleComboBox->setVisible(true);

    m_waitingRoleSelection = true;
    ui->loginButton->setText("Continue");

    resetTimeout();
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
    ui->roleLabel->setVisible(false);
    ui->roleComboBox->setVisible(false);
    ui->roleComboBox->clear();
    ui->cardNumberLineEdit->setEnabled(true);
    ui->pinLineEdit->setEnabled(true);

    m_waitingRoleSelection = false;
    m_accounts = QJsonArray();

    reject();
}

void LoginDialog::resetTimeout()
{
    // Käyttäjä teki toiminnon -> resetoi aikaraja
    if (m_loginInProgress) return;
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
    ui->roleLabel->setVisible(false);
    ui->roleComboBox->setVisible(false);
    ui->roleComboBox->clear();
    ui->cardNumberLineEdit->setEnabled(true);
    ui->pinLineEdit->setEnabled(true);

    m_waitingRoleSelection = false;
    m_accounts = QJsonArray();
    m_accountId = -1;
    m_accountRole = "debit";

    // Focus to a sensible field
    if (ui->cardNumberLineEdit->text().trimmed().isEmpty()) {
        ui->cardNumberLineEdit->setFocus();
    } else {
        ui->pinLineEdit->setFocus();
    }

    m_loginInProgress = false;
    resetTimeout();
}
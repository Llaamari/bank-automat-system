#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ApiClient.h"

#include <QMessageBox>
#include <QDateTime>
#include <QLocale>
#include <QShortcut>
#include <QTabBar>

MainWindow::MainWindow(ApiClient* api, int accountId, QWidget *parent)
    : MainWindow(api, accountId, QStringLiteral("debit"), parent)
{
}

MainWindow::MainWindow(ApiClient* api, int accountId, const QString& role, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_api(api),
      m_accountId(accountId),
      m_accountRole(role)
{
    ui->setupUi(this);

    const QString roleTxt = m_accountRole.isEmpty() ? QString("debit") : m_accountRole;
    setWindowTitle(QString("Bank Automat - %1 (Account %2)").arg(roleTxt, QString::number(m_accountId)));

    showFullScreen();   // koko ruutu

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(close()));

    // Setup transactions table
    ui->transactionsTable->setColumnCount(3);
    ui->transactionsTable->setHorizontalHeaderLabels({"Date", "Type", "Amount"});
    ui->transactionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->transactionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->transactionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->transactionsTable->horizontalHeader()->setStretchLastSection(true);

    // Connect ApiClient signals
    connect(m_api, &ApiClient::balanceResult,
            this, &MainWindow::onBalanceResult);

    connect(m_api, &ApiClient::withdrawResult,
            this, &MainWindow::onWithdrawResult);

    connect(m_api, &ApiClient::transactionsResult,
            this, &MainWindow::onTransactionsResult);

    // Initial load
    refreshAll();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;

    ui->refreshBalanceButton->setEnabled(!busy);
    ui->withdraw20Button->setEnabled(!busy);
    ui->withdraw40Button->setEnabled(!busy);
    ui->withdraw50Button->setEnabled(!busy);
    ui->withdraw100Button->setEnabled(!busy);
    ui->refreshTransactionsButton->setEnabled(!busy);

    if (busy) statusBar()->showMessage("Loading...");
    else statusBar()->clearMessage();
}

void MainWindow::setWithdrawError(const QString& msg)
{
    if (!ui->withdrawErrorLabel) return;
    ui->withdrawErrorLabel->setText(msg);
    ui->withdrawErrorLabel->setVisible(!msg.trimmed().isEmpty());
    ui->withdrawErrorLabel->setStyleSheet("color: red;");
}

void MainWindow::clearWithdrawError()
{
    setWithdrawError("");
}

void MainWindow::refreshAll()
{
    requestBalance();
    requestTransactions();
}

void MainWindow::requestBalance()
{
    setBusy(true);
    m_api->getBalance(m_accountId);
}

void MainWindow::requestTransactions()
{
    setBusy(true);
    m_api->getTransactions(m_accountId, 10);
}

void MainWindow::doWithdraw(int amount)
{
    clearWithdrawError();
    setBusy(true);
    m_api->withdraw(m_accountId, amount);
}

// -------- UI slots --------

void MainWindow::on_refreshBalanceButton_clicked()
{
    requestBalance();
}

void MainWindow::on_refreshTransactionsButton_clicked()
{
    requestTransactions();
}

void MainWindow::on_withdraw20Button_clicked()  { doWithdraw(20); }
void MainWindow::on_withdraw40Button_clicked()  { doWithdraw(40); }
void MainWindow::on_withdraw50Button_clicked()  { doWithdraw(50); }
void MainWindow::on_withdraw100Button_clicked() { doWithdraw(100); }

void MainWindow::on_customWithdrawButton_clicked()
{
    bool ok = false;
    const int amount = ui->customAmountLineEdit->text().trimmed().toInt(&ok);

    if (!ok || amount <= 0) {
        QMessageBox::warning(this, "Withdraw", "Enter a positive whole number.");
        return;
    }

    doWithdraw(amount); // käyttää samaa backend endpointtia
}

// -------- API result slots --------

void MainWindow::onBalanceResult(bool ok, QJsonObject data, QString error)
{
    setBusy(false);

    if (!ok) {
        QMessageBox::warning(this, "Balance", error.isEmpty() ? "Failed to load balance." : error);
        return;
    }

    updateBalanceUi(data);
}

void MainWindow::onWithdrawResult(bool ok, QJsonObject data, QString error)
{
    setBusy(false);

    if (!ok) {
        // Näytä withdraw-tabin virheet labelissa
        setWithdrawError(error.isEmpty() ? "Withdraw failed." : error);
        return;
    }

    clearWithdrawError();

    // Optional success info:
    const double newBalance = data.value("balance").toDouble();
    QString extra;

    if (data.contains("bills") && data.value("bills").isObject()) {
        const QJsonObject bills = data.value("bills").toObject();
        const int f = bills.value("50").toInt(0);
        const int t = bills.value("20").toInt(0);
        extra = QString("\nBills: 50€ x %1, 20€ x %2").arg(f).arg(t);
    }

    QMessageBox::information(this, "Withdraw",
                             QString("Withdraw successful.\nNew balance: %1%2")
                                 .arg(QLocale().toString(newBalance, 'f', 2), extra));

    // Refresh after withdraw
    refreshAll();
}

void MainWindow::onTransactionsResult(bool ok, QJsonArray data, QString error)
{
    setBusy(false);

    if (!ok) {
        QMessageBox::warning(this, "Transactions", error.isEmpty() ? "Failed to load transactions." : error);
        return;
    }

    updateTransactionsUi(data);
}

// -------- UI update helpers --------

void MainWindow::updateBalanceUi(const QJsonObject &data)
{
    // Backend returns strings for DECIMAL often; handle both string/double
    const QString balanceStr = data.value("balance").toVariant().toString();
    const QString typeStr = data.value("account_type").toString("debit");

    const QString modeStr = m_accountRole.isEmpty() ? QString("debit") : m_accountRole;
    ui->balanceLabel->setText(QString("Mode: %1 | Balance: %2 € (%3)")
                                  .arg(modeStr, balanceStr, typeStr));
}

void MainWindow::updateTransactionsUi(const QJsonArray &rows)
{
    ui->transactionsTable->setRowCount(0);

    for (int i = 0; i < rows.size(); ++i) {
        const QJsonObject obj = rows[i].toObject();
        const QString createdAt = obj.value("created_at").toString();
        const QString txType = obj.value("tx_type").toString();
        const QString amount = obj.value("amount").toVariant().toString();

        ui->transactionsTable->insertRow(i);

        // Date formatting (best-effort)
        QString dateText = createdAt;
        // If backend returns "YYYY-MM-DDTHH:MM:SS..." or "YYYY-MM-DD HH:MM:SS"
        QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
        if (!dt.isValid()) dt = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss");
        if (dt.isValid()) dateText = QLocale().toString(dt, QLocale::ShortFormat);

        ui->transactionsTable->setItem(i, 0, new QTableWidgetItem(dateText));
        ui->transactionsTable->setItem(i, 1, new QTableWidgetItem(txType));
        ui->transactionsTable->setItem(i, 2, new QTableWidgetItem(amount));
    }

    ui->transactionsTable->resizeColumnsToContents();
}
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ApiClient.h"

#include <QMessageBox>
#include <QDateTime>
#include <QLocale>
#include <QShortcut>
#include <QTabBar>
#include <QTabWidget>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QResizeEvent>

static constexpr int IDLE_TIMEOUT_MS = 30 * 1000;

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

    // Image UI init
    showImagePlaceholder(QStringLiteral("No image"));

    // Fetch customer's image filename via account -> customer, then fetch image bytes
    if (m_api) {
        showImagePlaceholder(QStringLiteral("Loading..."));
        m_api->getCustomerImageFilenameForAccount(m_accountId,
            [this](bool ok, const QString& filename, const QString& error) {
                if (!ok) {
                    showImagePlaceholder(QStringLiteral("No image"));
                    return;
                }

                const QString fn = filename.trimmed();
                if (fn.isEmpty()) {
                    showImagePlaceholder(QStringLiteral("No image"));
                    return;
                }

                m_api->fetchImageByFilename(fn,
                    [this](const QByteArray& data) {
                        setImageFromBytes(data);
                    },
                    [this](const QString&) {
                        showImagePlaceholder(QStringLiteral("Image not found"));
                    }
                );
            }
        );
    }

    static constexpr int IDLE_TIMEOUT_MS = 30 * 1000;
    m_idleTimer.setInterval(IDLE_TIMEOUT_MS);
    m_idleTimer.setSingleShot(true);

    connect(&m_idleTimer, &QTimer::timeout, this, [this]() {
        emit idleTimeout();
    });

    // Listen to all app events -> reset timer on any user activity
    qApp->installEventFilter(this);
    resetIdleTimer();

    const QString roleTxt = m_accountRole.isEmpty() ? QString("debit") : m_accountRole;
    setWindowTitle(QString("Bank Automat - %1 (Account %2)").arg(roleTxt, QString::number(m_accountId)));

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(close()));

    // Setup transactions table
    ui->transactionsTable->setColumnCount(3);
    ui->transactionsTable->setHorizontalHeaderLabels({"Date", "Type", "Amount"});
    ui->transactionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->transactionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->transactionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->transactionsTable->horizontalHeader()->setStretchLastSection(true);

    // Tabs: show "No transactions" only when the user actually opens the Transactions tab
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::on_tabWidget_currentChanged);

    // Connect ApiClient signals
    connect(m_api, &ApiClient::balanceResult,
            this, &MainWindow::onBalanceResult);

    connect(m_api, &ApiClient::withdrawResult,
            this, &MainWindow::onWithdrawResult);

    connect(m_api, &ApiClient::transactionsPageResult,
            this, &MainWindow::onTransactionsPageResult);

    // Initial load
    refreshAll();
}

MainWindow::~MainWindow()
{
    qApp->removeEventFilter(this);
    delete ui;
}

void MainWindow::resetIdleTimer()
{
    m_idleTimer.start();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::InputMethod:
    case QEvent::FocusIn:
        resetIdleTimer();
        break;
    default:
        break;
    }
    return false; // don't swallow events
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

    if (ui->prevTransactionsButton) ui->prevTransactionsButton->setEnabled(!busy && !m_prevCursor.isEmpty());
    if (ui->nextTransactionsButton) ui->nextTransactionsButton->setEnabled(!busy && !m_nextCursor.isEmpty());

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
    requestTransactionsFirstPage();
}

void MainWindow::requestBalance()
{
    setBusy(true);
    m_api->getBalance(m_accountId);
}

void MainWindow::requestTransactionsFirstPage()
{
    setBusy(true);

    m_txPageIndex = 0;
    m_txHistory.clear();
    m_nextCursor.clear();
    m_prevCursor.clear();
    m_lastTxMove = TxMove::First;
    m_hasAnyTransactions = false;
    m_noTransactionsPopupShown = false;
    updateTransactionsNavUi();

    m_api->getTransactionsPage(m_accountId, TX_PAGE_SIZE);
}

void MainWindow::requestTransactionsBefore(const QString& beforeCursor)
{
    setBusy(true);
    m_api->getTransactionsPage(m_accountId, TX_PAGE_SIZE, beforeCursor, QString());
}

void MainWindow::requestTransactionsAfter(const QString& afterCursor)
{
    setBusy(true);
    m_api->getTransactionsPage(m_accountId, TX_PAGE_SIZE, QString(), afterCursor);
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
    requestTransactionsFirstPage();
}

void MainWindow::on_prevTransactionsButton_clicked()
{
    if (m_busy) return;
    if (m_txPageIndex == 0) return;     // <-- prevents "newer than newest" empty page
    if (m_prevCursor.isEmpty()) return;

    m_txPageIndex -= 1;
    m_lastTxMove = TxMove::Prev;

    // Restore cursors for the target page (the page we are returning to)
    if (!m_txHistory.isEmpty()) {
        const auto target = m_txHistory.takeLast();
        // We can't set them fully yet (we still need items), but we can restore nextCursor immediately
        // so Next won't get disabled after returning.
        m_nextCursor = target.nextCursor;
        // prevCursor will be set from response (or keep target.prevCursor)
    }

    requestTransactionsAfter(m_prevCursor);  // newer items
}

void MainWindow::on_nextTransactionsButton_clicked()
{
    if (m_busy || m_nextCursor.isEmpty()) return;
    m_txHistory.push_back({ m_nextCursor, m_prevCursor }); // save current page state
    m_txPageIndex += 1;
    m_lastTxMove = TxMove::Next;
    // Next = older items
    requestTransactionsBefore(m_nextCursor);
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

    // Refresh after withdraw -> first page so the new tx is visible
    refreshAll();
}

void MainWindow::onTransactionsResult(bool ok, QJsonArray data, QString error)
{
    // kept for compatibility (not connected in current setup)
    setBusy(false);

    if (!ok) {
        QMessageBox::warning(this, "Transactions", error.isEmpty() ? "Failed to load transactions." : error);
        return;
    }

    updateTransactionsUi(data);
}

void MainWindow::onTransactionsPageResult(bool ok, QJsonArray items,
                                         QString nextCursor, QString prevCursor,
                                         QString error)
{
    setBusy(false);

    if (!ok) {
        QMessageBox::warning(this, "Transactions", error.isEmpty() ? "Failed to load transactions." : error);
        updateTransactionsNavUi();
        return;
    }

    // Empty result handling:
    // - On initial load (First page) an empty list means: there are no transactions for this account.
    //   Do NOT show any popup unless the user is on the Transactions tab.
    // - On Next/Prev navigation an empty list means: you've reached the end in that direction.
    if (items.isEmpty()) {
        if (m_lastTxMove == TxMove::First) {
            // No transactions at all -> keep UI calm on login.
            m_hasAnyTransactions = false;
            m_nextCursor.clear();
            m_prevCursor.clear();
            m_lastTxMove = TxMove::None;
            updateTransactionsUi(QJsonArray());
            updateTransactionsNavUi();
            return;
        }

        // Navigation beyond available pages.
        if (ui->tabWidget->currentIndex() == 2) {
            QMessageBox::information(this, "Transactions", "No more transactions in that direction.");
        }

        if (m_txPageIndex < 0) m_txPageIndex = 0;
        updateTransactionsNavUi();
        return;
    }

    m_hasAnyTransactions = true;
    m_noTransactionsPopupShown = false;

    // Only overwrite nextCursor if server gave one.
    // This prevents "Next" becoming disabled after returning with Prev.
    if (!nextCursor.isEmpty()) {
        m_nextCursor = nextCursor;
    }
    // prevCursor can always be overwritten
    m_prevCursor = prevCursor;

    // nextCursor handling:
    // - If we moved NEXT (older) or FIRST page: trust server, even if empty -> disables Next at end
    // - If we moved PREV (newer): server may not provide a meaningful nextCursor for "older" direction,
    //   so keep whatever we restored from history unless server gives a non-empty value.
    if (m_lastTxMove == TxMove::Prev) {
        if (!nextCursor.isEmpty()) {
            m_nextCursor = nextCursor;
        }
    } else {
        // First/Next/None: overwrite even if empty
        m_nextCursor = nextCursor;
    }

    m_lastTxMove = TxMove::None;
    updateTransactionsUi(items);
    updateTransactionsNavUi();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    // Transactions tab index is 2 (Balance=0, Withdraw=1, Transactions=2)
    if (index != 2) return;

    // If there are no transactions, show a single informative popup when the user opens the tab.
    if (!m_busy && !m_hasAnyTransactions && ui->transactionsTable->rowCount() == 0 && !m_noTransactionsPopupShown) {
        m_noTransactionsPopupShown = true;
        QMessageBox::information(this, "Transactions", "No transactions.");
    }
}

void MainWindow::updateTransactionsNavUi()
{
    const bool canPrev = (!m_busy) && (m_txPageIndex > 0) && (!m_prevCursor.isEmpty());
    const bool canNext = (!m_busy) && (!m_nextCursor.isEmpty());

    if (ui->prevTransactionsButton) ui->prevTransactionsButton->setEnabled(canPrev);
    if (ui->nextTransactionsButton) ui->nextTransactionsButton->setEnabled(canNext);
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

    const int startNumber = (m_txPageIndex * TX_PAGE_SIZE) + 1;

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

        // Set running row number in the vertical header: 1-10, 11-20, ...
        ui->transactionsTable->setVerticalHeaderItem(
            i, new QTableWidgetItem(QString::number(startNumber + i))
        );
    }

    ui->transactionsTable->resizeColumnsToContents();
}

void MainWindow::showImagePlaceholder(const QString& text)
{
    if (!ui || !ui->imageLabel) return;
    ui->imageLabel->clear();
    ui->imageLabel->setText(text);
    m_originalPixmap = QPixmap();
}

void MainWindow::setImageFromBytes(const QByteArray& data)
{
    QPixmap pix;
    if (!pix.loadFromData(data)) {
        showImagePlaceholder(QStringLiteral("Image decode failed"));
        return;
    }
    m_originalPixmap = pix;
    rescaleImageToLabel();
}

void MainWindow::rescaleImageToLabel()
{
    if (!ui || !ui->imageLabel) return;
    if (m_originalPixmap.isNull()) return;

    const QSize target = ui->imageLabel->size();
    const QPixmap scaled = m_originalPixmap.scaled(
        target,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    ui->imageLabel->setPixmap(scaled);
    ui->imageLabel->setAlignment(Qt::AlignCenter);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    rescaleImageToLabel();
}

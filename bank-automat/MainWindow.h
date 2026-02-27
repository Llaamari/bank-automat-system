#pragma once

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QEvent>
#include <QVector>

class ApiClient;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ApiClient* api, int accountId, QWidget *parent = nullptr);
    explicit MainWindow(ApiClient* api, int accountId, const QString& role, QWidget *parent = nullptr);
    ~MainWindow();

signals:
    // Emitted after 30 seconds of inactivity. StartWindow should handle returning to start.
    void idleTimeout();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // UI events
    void on_refreshBalanceButton_clicked();
    void on_withdraw20Button_clicked();
    void on_withdraw40Button_clicked();
    void on_withdraw50Button_clicked();
    void on_withdraw100Button_clicked();
    void on_refreshTransactionsButton_clicked();
    void on_prevTransactionsButton_clicked();
    void on_nextTransactionsButton_clicked();
    void on_customWithdrawButton_clicked();

    // Tabs
    void on_tabWidget_currentChanged(int index);

    // API results
    void onBalanceResult(bool ok, QJsonObject data, QString error);
    void onWithdrawResult(bool ok, QJsonObject data, QString error);
    void onTransactionsResult(bool ok, QJsonArray data, QString error);
    void onTransactionsPageResult(bool ok, QJsonArray items,
                                  QString nextCursor, QString prevCursor,
                                  QString error);

private:
    Ui::MainWindow *ui;
    ApiClient* m_api = nullptr;
    int m_accountId = -1;
    QString m_accountRole = "debit";

    void refreshAll();
    void requestBalance();

    void requestTransactionsFirstPage();
    void requestTransactionsBefore(const QString& beforeCursor);
    void requestTransactionsAfter(const QString& afterCursor);

    void doWithdraw(int amount);

    void setBusy(bool busy);
    void updateBalanceUi(const QJsonObject& data);
    void updateTransactionsUi(const QJsonArray& rows);
    void updateTransactionsNavUi();

    void setWithdrawError(const QString& msg);
    void clearWithdrawError();

    bool m_busy = false;

    // 30s inactivity handling
    void resetIdleTimer();
    QTimer m_idleTimer;
    static constexpr int TX_PAGE_SIZE = 10;
    QString m_nextCursor;
    QString m_prevCursor;
    int m_txPageIndex = 0; // 0 = newest page, 1 = next older page, ...

    struct TxPageCursors {
        QString nextCursor;
        QString prevCursor;
    };
    QVector<TxPageCursors> m_txHistory; // stack of pages (page 0,1,2...)

    enum class TxMove { None, First, Next, Prev };
    TxMove m_lastTxMove = TxMove::None;

    // Used to avoid showing a transactions popup on login when the user is not on the Transactions tab.
    bool m_hasAnyTransactions = false;
    bool m_noTransactionsPopupShown = false;
};
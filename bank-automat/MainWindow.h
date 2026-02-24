#pragma once

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonArray>

class ApiClient;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ApiClient* api, int accountId, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // UI events
    void on_refreshBalanceButton_clicked();
    void on_withdraw20Button_clicked();
    void on_withdraw40Button_clicked();
    void on_withdraw50Button_clicked();
    void on_withdraw100Button_clicked();
    void on_refreshTransactionsButton_clicked();

    // API results
    void onBalanceResult(bool ok, QJsonObject data, QString error);
    void onWithdrawResult(bool ok, QJsonObject data, QString error);
    void onTransactionsResult(bool ok, QJsonArray data, QString error);

private:
    Ui::MainWindow *ui;
    ApiClient* m_api = nullptr;
    int m_accountId = -1;

    void refreshAll();
    void requestBalance();
    void requestTransactions();
    void doWithdraw(int amount);

    void setBusy(bool busy);
    void updateBalanceUi(const QJsonObject& data);
    void updateTransactionsUi(const QJsonArray& rows);

    bool m_busy = false;
};
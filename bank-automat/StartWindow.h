#pragma once

#include <QWidget>

class ApiClient;
class MainWindow;

QT_BEGIN_NAMESPACE
namespace Ui { class StartWindow; }
QT_END_NAMESPACE

class StartWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StartWindow(ApiClient* api, QWidget *parent = nullptr);
    ~StartWindow();

public slots:
    // Return to the initial UI state (used by inactivity timeout and window close)
    void forceResetToStart();

private slots:
    void on_startButton_clicked();

private:
    Ui::StartWindow *ui;
    ApiClient* m_api = nullptr;
    MainWindow* m_mainWindow = nullptr;
};
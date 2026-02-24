#pragma once

#include <QWidget>

class ApiClient;

QT_BEGIN_NAMESPACE
namespace Ui { class StartWindow; }
QT_END_NAMESPACE

class StartWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StartWindow(ApiClient* api, QWidget *parent = nullptr);
    ~StartWindow();

private slots:
    void on_startButton_clicked();

private:
    Ui::StartWindow *ui;
    ApiClient* m_api = nullptr;
};
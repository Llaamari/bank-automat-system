#include "StartWindow.h"
#include "ui_StartWindow.h"

#include "ApiClient.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include <QShortcut>
#include <QKeySequence>
#include <QTimer>

StartWindow::StartWindow(ApiClient* api, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::StartWindow),
      m_api(api)
{
    ui->setupUi(this);
    setWindowTitle("Bank Automat");

    showFullScreen();   // koko ruutu

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(close()));
}

StartWindow::~StartWindow()
{
    delete ui;
}

void StartWindow::on_startButton_clicked()
{
    LoginDialog dlg(m_api, this);

    if (dlg.exec() == QDialog::Accepted) {
        const int accountId = dlg.accountId();

        auto *mw = new MainWindow(m_api, accountId);
        mw->showFullScreen();

        // Sulje StartWindow vasta seuraavalla event loop -kierroksella
        QTimer::singleShot(50, this, [this]() { this->close(); });
    }
}
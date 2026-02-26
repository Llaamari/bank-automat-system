#include "StartWindow.h"
#include "ui_StartWindow.h"

#include "ApiClient.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include <QShortcut>
#include <QKeySequence>
#include <QTimer>
#include <QApplication>

static constexpr int HANDOFF_POLL_MS = 15;
static constexpr int HANDOFF_MAX_MS  = 1500;

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

void StartWindow::forceResetToStart()
{
    // Bring StartWindow up FIRST (cover desktop)
    this->showFullScreen();
    this->raise();
    this->activateWindow();
    if (ui && ui->startButton) ui->startButton->setFocus();

    // Then close MainWindow slightly later (avoid desktop flash)
    if (m_mainWindow) {
        MainWindow* toClose = m_mainWindow;
        m_mainWindow = nullptr;

        QTimer::singleShot(50, this, [toClose]() {
            if (toClose) {
                toClose->close();
                toClose->deleteLater();
            }
        });
    }
}

void StartWindow::on_startButton_clicked()
{
    LoginDialog dlg(m_api, this);

    if (dlg.exec() == QDialog::Accepted) {
        const int accountId = dlg.accountId();
        const QString role = dlg.accountRole();

        // Make absolutely sure StartWindow covers the screen right after the modal closes
        this->showFullScreen();
        this->raise();
        this->activateWindow();
        qApp->processEvents(); // force paint before creating MainWindow

        // Create + show MainWindow first. Keep StartWindow visible until
        // MainWindow becomes the active top-level window to avoid a brief
        // desktop "flash" when the modal dialog closes.
        m_mainWindow = new MainWindow(m_api, accountId, role);

        // If MainWindow emits inactivity timeout, return to start
        QObject::connect(m_mainWindow, SIGNAL(idleTimeout()), this, SLOT(forceResetToStart()));

        // If user closes MainWindow manually, return to start
        connect(m_mainWindow, &QObject::destroyed, this, [this]() {
            m_mainWindow = nullptr;
        });

        m_mainWindow->showFullScreen();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();

        // Poll until MainWindow is active, then hide StartWindow.
        auto *handoffTimer = new QTimer(this);
        handoffTimer->setInterval(HANDOFF_POLL_MS);
        handoffTimer->setSingleShot(false);

        QTimer::singleShot(HANDOFF_MAX_MS, handoffTimer, [handoffTimer]() {
            if (handoffTimer) handoffTimer->stop();
        });

        connect(handoffTimer, &QTimer::timeout, this, [this, handoffTimer]() {
            if (!m_mainWindow) {
                handoffTimer->stop();
                handoffTimer->deleteLater();
                return;
            }

            m_mainWindow->raise();
            m_mainWindow->activateWindow();

            if (m_mainWindow->isVisible() && m_mainWindow->isActiveWindow()) {
                this->hide();
                handoffTimer->stop();
                handoffTimer->deleteLater();
            }
        });

        handoffTimer->start();
    } else {
        this->showFullScreen();
        this->raise();
        this->activateWindow();
        if (ui && ui->startButton) ui->startButton->setFocus();
    }
}
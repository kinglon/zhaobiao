#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);    

    initController();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initController()
{
    m_ipcWorker.setKey(IPC_KEY);
    connect(&m_ipcWorker, &IpcWorker::ipcDataArrive, this, &MainWindow::onIpcDataArrive);
    m_ipcWorker.start();

    connect(&m_updateController, &UpdateController::printLog, this, &MainWindow::onPrintLog);
    m_updateController.run();
}

void MainWindow::onPrintLog(QString content)
{
    static int lineCount = 0;
    if (lineCount >= 1000)
    {
        ui->textEditLog->clear();
        lineCount = 0;
    }
    lineCount++;

    qInfo(content.toStdString().c_str());
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentTimeString = currentDateTime.toString("[MM-dd hh:mm:ss] ");
    QString line = currentTimeString + content;
    ui->textEditLog->append(line);
}

void MainWindow::onIpcDataArrive(QString data)
{
    if (data.contains("showWindow"))
    {
        show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingmanager.h"
#include <QDateTime>
#include <QDesktopServices>
#include "zhaobiaohttpclient.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    QShortcut* ctrlDShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(ctrlDShortcut, &QShortcut::activated, this, &MainWindow::onCtrlDShortcut);

    initCtrls();
    initController();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCtrls()
{
    ui->comboBoxKeyWord->clear();
    for (const auto& keyWord : SettingManager::getInstance()->m_filterKeyWords)
    {
        ui->comboBoxKeyWord->addItem(keyWord.m_name);
    }

    connect(ui->comboBoxKeyWord, &QComboBox::currentTextChanged, this, &MainWindow::onKeyWordComboCurrentTextChanged);
    connect(ui->pushButtonLogin, &QPushButton::clicked, this, &MainWindow::onLoginButtonClicked);
    connect(ui->pushButtonStart, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(ui->pushButtonStop, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(ui->pushButtonOpenFolder, &QPushButton::clicked, this, &MainWindow::onOpenFolderButtonClicked);

    updateButtonStatus();
}

void MainWindow::updateButtonStatus()
{
    ui->pushButtonLogin->setEnabled(!m_collecting);
    ui->pushButtonStop->setEnabled(m_collecting);
    ui->pushButtonOpenFolder->setEnabled(!m_lastSavedPath.isEmpty());
}

void MainWindow::initController()
{
    connect(&m_loginController, &LoginController::printLog, this, &MainWindow::onPrintLog);
}

void MainWindow::onCtrlDShortcut()
{
    ZhaoBiaoHttpClient::test();
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

void MainWindow::onLoginButtonClicked()
{
    m_loginController.run();
}

void MainWindow::onStartButtonClicked()
{
    m_collectController = new CollectController();
    connect(m_collectController, &CollectController::printLog, this, &MainWindow::onPrintLog);
    connect(m_collectController, &CollectController::runFinish, [this](bool success, QString savedPath) {
        m_collecting = false;
        if (success)
        {
            m_lastSavedPath = savedPath;
        }
        updateButtonStatus();
        m_collectController->deleteLater();
        m_collectController = nullptr;
    });
    m_collecting = true;
    updateButtonStatus();
    m_collectController->run();
}

void MainWindow::onStopButtonClicked()
{
    if (m_collectController)
    {
        m_collectController->stop();
    }
}

void MainWindow::onOpenFolderButtonClicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastSavedPath));
}

void MainWindow::onKeyWordComboCurrentTextChanged(QString text)
{
    for (auto& keyWord : SettingManager::getInstance()->m_filterKeyWords)
    {
        if (keyWord.m_name == text)
        {
            ui->lineEditTitleKeyWord->setText(keyWord.m_titleKeyWord);
            ui->lineEditContentKeyWord->setText(keyWord.m_contentKeyWord);
            break;
        }
    }
}

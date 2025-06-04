#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingmanager.h"
#include <QDateTime>
#include <QDesktopServices>
#include "zhaobiaohttpclient.h"
#include "browserwindow.h"
#include "statusmanager.h"
#include "uiutil.h"
#include "update/common/updateutil.h"

#define HOME_PAGE "https://www.zhaobiao.cn/"

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
    initBrowser();
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
        ui->comboBoxKeyWord->addItem(keyWord.m_type);
    }
    ui->comboBoxKeyWord->setCurrentIndex(-1);

    connect(ui->comboBoxKeyWord, &QComboBox::currentTextChanged, this, &MainWindow::onKeyWordComboCurrentTextChanged);
    connect(ui->pushButtonLogin, &QPushButton::clicked, this, &MainWindow::onLoginButtonClicked);
    connect(ui->pushButtonStart, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(ui->pushButtonStop, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(ui->pushButtonOpenFolder, &QPushButton::clicked, this, &MainWindow::onOpenFolderButtonClicked);

    updateButtonStatus();
}

void MainWindow::initBrowser()
{
    BrowserWindow::getInstance()->setHideWhenClose(true);
}

void MainWindow::updateButtonStatus()
{
    ui->pushButtonStart->setEnabled(!m_collecting);
    ui->pushButtonStop->setEnabled(m_collecting);
    ui->pushButtonOpenFolder->setEnabled(!m_lastSavedPath.isEmpty());
}

void MainWindow::initController()
{
    // 启动升级程序，后台运行
    QStringList arguments;
    arguments.append("hide");
    UpdateUtil::startUpgradeProgram("", arguments);

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
    BrowserWindow::getInstance()->load(QUrl(HOME_PAGE));
    BrowserWindow::getInstance()->showMaximized();
    m_loginController.run();
}

void MainWindow::onStartButtonClicked()
{
    // 检测是否有新版本
    QString currentVersion;
    QString newVersion;
    if (UpdateUtil::needUpgrade(newVersion, currentVersion))
    {
        UiUtil::showTip(QString::fromWCharArray(L"检测到新版本，请升级"));
        UpdateUtil::startUpgradeProgram("", QStringList());
        close();
        return;
    }

    if (StatusManager::getInstance()->getCookies().isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请先登录"));
        return;
    }

    FilterKeyWord filterKeyWord;
    filterKeyWord.m_type = ui->comboBoxKeyWord->currentText();
    filterKeyWord.m_titleKeyWord = ui->lineEditTitleKeyWord->text();
    filterKeyWord.m_contentKeyWord = ui->lineEditContentKeyWord->text();
    if (filterKeyWord.m_titleKeyWord.isEmpty() || filterKeyWord.m_contentKeyWord.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请先选择关键词"));
        return;
    }
    StatusManager::getInstance()->setCurrentFilterKeyWord(filterKeyWord);

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
        if (keyWord.m_type == text)
        {
            ui->lineEditTitleKeyWord->setText(keyWord.m_titleKeyWord);
            ui->lineEditTitleKeyWord->setCursorPosition(0);
            ui->lineEditContentKeyWord->setText(keyWord.m_contentKeyWord);
            ui->lineEditContentKeyWord->setCursorPosition(0);
            break;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    BrowserWindow::getInstance()->setCanClose();
    BrowserWindow::getInstance()->close();
    event->accept();
}

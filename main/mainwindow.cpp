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
    ui->lineEditUserName->setText(SettingManager::getInstance()->m_userName);
    ui->lineEditPassword->setText(SettingManager::getInstance()->m_password);

    QDateTime searchEndDate = QDateTime::currentDateTime();
    if (SettingManager::getInstance()->m_searchEndDate > 0)
    {
        searchEndDate = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchEndDate);
    }
    ui->dateEditSearchEnd->setDate(searchEndDate.date());

    QDateTime searchBeginDate = searchEndDate.addDays(-3);
    if (SettingManager::getInstance()->m_searchBeginDate > 0)
    {
        searchBeginDate = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchBeginDate);
    }
    ui->dateEditSearchBegin->setDate(searchBeginDate.date());

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

void MainWindow::saveSetting()
{
    SettingManager::getInstance()->m_userName = ui->lineEditUserName->text();
    SettingManager::getInstance()->m_password = ui->lineEditPassword->text();
    SettingManager::getInstance()->m_searchBeginDate = ui->dateEditSearchBegin->dateTime().toSecsSinceEpoch();
    SettingManager::getInstance()->m_searchEndDate = ui->dateEditSearchEnd->dateTime().toSecsSinceEpoch();
    SettingManager::getInstance()->save();
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
    saveSetting();    
    m_loginController.run();
}

void MainWindow::onStartButtonClicked()
{
    saveSetting();

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

    StatusManager::getInstance()->m_downloadAttachment = ui->checkBoxDownloadAttachment->isChecked();

    // 获取搜索关键词列表
    QCheckBox* keyWordCheckBoxes[4] = {
        ui->checkBoxKuangShan, ui->checkBoxBaoPoShiGong,
        ui->checkBoxBaoPoFuWu, ui->checkBoxDiZhiZaiHai
    };
    StatusManager::getInstance()->m_searchFilterKeyWords.clear();
    for (int i=0; i<sizeof(keyWordCheckBoxes)/sizeof(keyWordCheckBoxes[0]); i++)
    {
        if (keyWordCheckBoxes[i]->isChecked())
        {
            QString type = keyWordCheckBoxes[i]->text();
            for (const auto& keyWord : SettingManager::getInstance()->m_filterKeyWords)
            {
                if (type == keyWord.m_type)
                {
                    StatusManager::getInstance()->m_searchFilterKeyWords.append(keyWord);
                    break;
                }
            }
        }
    }
    if (StatusManager::getInstance()->m_searchFilterKeyWords.empty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请选择要采集的关键词"));
        return;
    }

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

void MainWindow::closeEvent(QCloseEvent *event)
{
    BrowserWindow::getInstance()->setCanClose();
    BrowserWindow::getInstance()->close();
    event->accept();
}

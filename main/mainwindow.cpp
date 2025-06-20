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
    ui->lineEditPriorityProvince->setText(SettingManager::getInstance()->m_priorityRegions);

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

    // 启动定时器
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimer);
    timer->start(1000);
}

void MainWindow::saveSetting()
{    
    QDateTime dateTime;
    dateTime.setDate(QDate::currentDate());
    SettingManager::getInstance()->m_searchEndDate = dateTime.toSecsSinceEpoch();
    dateTime.setDate(QDate::currentDate().addDays(-1));
    SettingManager::getInstance()->m_searchBeginDate = dateTime.toSecsSinceEpoch();

    SettingManager::getInstance()->m_userName = ui->lineEditUserName->text();
    SettingManager::getInstance()->m_password = ui->lineEditPassword->text();
    SettingManager::getInstance()->m_priorityRegions = ui->lineEditPriorityProvince->text();
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

void MainWindow::doCollect()
{
    StatusManager::getInstance()->m_downloadAttachment = ui->checkBoxDownloadAttachment->isChecked();
    saveSetting();
    m_nextCollectTime = 0;

    m_collectController = new CollectController();
    connect(m_collectController, &CollectController::printLog, this, &MainWindow::onPrintLog);
    connect(m_collectController, &CollectController::runFinish, [this](bool success, QString savedPath) {
        m_collectController->deleteLater();
        m_collectController = nullptr;

        if (success)
        {
            m_lastSavedPath = savedPath;

            // 明天8点半自动采集
            QDateTime nextCollectDateTime;
            nextCollectDateTime.setDate(QDate::currentDate().addDays(1));
            nextCollectDateTime.setTime(QTime(8, 30));
            m_nextCollectTime = nextCollectDateTime.toSecsSinceEpoch();
        }
        else
        {
            m_collecting = false;
        }
        updateButtonStatus();
    });
    m_collecting = true;
    updateButtonStatus();
    m_collectController->run();
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

    doCollect();
}

void MainWindow::onStopButtonClicked()
{
    m_nextCollectTime = 0;

    if (m_collectController)
    {
        m_collectController->stop();
    }
    else
    {
        m_collecting = false;
        updateButtonStatus();
    }
}

void MainWindow::onOpenFolderButtonClicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastSavedPath));
}

void MainWindow::onTimer()
{
    if (m_nextCollectTime > 0)
    {
        qint64 now = QDateTime::currentSecsSinceEpoch();
        if (now >= m_nextCollectTime)
        {
            onPrintLog(QString::fromWCharArray(L"开始自动采集"));
            doCollect();
        }
        else
        {
            // 每隔1分钟提醒一次
            int elpase = m_nextCollectTime-now;
            if (elpase % 60 == 0)
            {
                QString nextCollectTimeString = QDateTime::fromSecsSinceEpoch(m_nextCollectTime).toString("yyyy-MM-dd hh:mm:ss");
                QString hour = QString::number(elpase / 3600);
                QString min = QString::number(elpase % 3600 / 60);
                QString second = QString::number(elpase % 60);
                QString message = QString::fromWCharArray(L"下次采集时间：%1，还剩：%2时%3分%4秒").arg(nextCollectTimeString, hour, min, second);
                onPrintLog(message);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    BrowserWindow::getInstance()->setCanClose();
    BrowserWindow::getInstance()->close();
    event->accept();
}

#include "logincontroller.h"
#include "browserwindow.h"
#include "jscodemanager.h"
#include "statusmanager.h"
#include "zhaobiaohttpclient.h"

#define LOGIN_PAGE "https://user.zhaobiao.cn/login.html"

LoginController::LoginController(QObject *parent)
    : QObject{parent}
{

}

void LoginController::run()
{
    if (m_running)
    {
        return;
    }

    m_running = true;
    m_needFillUserName = true;

    BrowserWindow::getInstance()->load(QUrl(LOGIN_PAGE));
    BrowserWindow::getInstance()->showMaximized();

    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished, this, &LoginController::onRunJsCodeFinished);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LoginController::onTimer);
    m_timer->start(1000);
}

void LoginController::stop()
{
    m_running = false;

    if (!SettingManager::getInstance()->m_debug)
    {
        BrowserWindow::getInstance()->close();
    }
    disconnect(BrowserWindow::getInstance(), nullptr, this, nullptr);

    if (m_timer)
    {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
}

void LoginController::startRefreshLoginInfo()
{
    if (m_refreshTimer)
    {
        return;
    }

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, [this]() {
        static qint64 lastRefreshTime = QDateTime::currentSecsSinceEpoch();
        qint64 now = QDateTime::currentSecsSinceEpoch();
        if (now - lastRefreshTime >= 1800)
        {
            // 检索一次，登录就不会过期
            emit printLog(QString::fromWCharArray(L"开始检索保持登录在线"));

            ZhaoBiaoHttpClient client;
            client.m_cookies = StatusManager::getInstance()->getCookies();

            QDate today = QDate::currentDate();
            SearchCondition condition;
            condition.m_beginDate = today.toString("yyyy-MM-dd");
            condition.m_endDate = condition.m_beginDate;
            condition.m_onlyTitleField = false;
            condition.m_province = SettingManager::getInstance()->m_searchProvince;
            condition.m_keyWord = QString::fromWCharArray(L"矿山");
            condition.m_channels = "bidding%2Cfore%2Cpurchase%2Cfree%2Crecommend%2Cproposed%2Centrust%2Capproved%2Ccommerce%2Clisted%2Cproperty%2Cmineral%2Cland%2Cauction%2Cother";
            condition.m_page = 1;

            QVector<ZhaoBiao> zhaoBiaos;
            int totalPage = 0;
            if (!client.search(condition, totalPage, zhaoBiaos))
            {
                emit printLog(client.m_lastError);
            }
            lastRefreshTime = QDateTime::currentSecsSinceEpoch();
        }
    });
    m_refreshTimer->start(1000);
}

void LoginController::onTimer()
{
    BrowserWindow::getInstance()->runJsCode("GetLoginInfo", JsCodeManager::getInstance()->m_getLoginInfo);
    qDebug("get login information");

    if (m_needFillUserName)
    {
        QString jsCode = JsCodeManager::getInstance()->m_fillUserPassword;
        jsCode.replace("$USERNAME", SettingManager::getInstance()->m_userName);
        jsCode.replace("$PASSWORD", SettingManager::getInstance()->m_password);
        BrowserWindow::getInstance()->runJsCode("fillUserPassword", jsCode);
    }
}

void LoginController::onRunJsCodeFinished(const QString& id, const QVariant& result)
{
    if (id == "GetLoginInfo")
    {
        QString cookies = result.toString();
        if (!cookies.isEmpty())
        {
            qInfo("cookies: %s", cookies.toStdString().c_str());
            StatusManager::getInstance()->setCookies(cookies);
            emit printLog(QString::fromWCharArray(L"已登录"));
            stop();

            // 开启刷新登录定时器
            startRefreshLoginInfo();
        }
    }
    else if (id == "fillUserPassword")
    {
        if (result.toInt() == 1)
        {
            qInfo("successful to fill username and password");
            m_needFillUserName = false;
        }
    }
}

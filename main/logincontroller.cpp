#include "logincontroller.h"
#include "browserwindow.h"
#include "jscodemanager.h"
#include "statusmanager.h"

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

void LoginController::onTimer()
{
    BrowserWindow::getInstance()->runJsCode("GetLoginInfo", JsCodeManager::getInstance()->m_getLoginInfo);
    qDebug("get login information");
}

void LoginController::onRunJsCodeFinished(const QString& id, const QVariant& result)
{
    if (id != "GetLoginInfo")
    {
        return;
    }

    QString cookies = result.toString();
    if (!cookies.isEmpty())
    {
        qInfo("cookies: %s", cookies.toStdString().c_str());
        StatusManager::getInstance()->setCookies(cookies);
        emit printLog(QString::fromWCharArray(L"已登录"));
        stop();
    }
}

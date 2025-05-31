#include "browserwindow.h"
#include "ui_browserwindow.h"
#include <QImage>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include "Utility/ImPath.h"

#define DEFAULT_PROFILE_NAME    "Default"

QWebEnginePage* WebEnginePage::createWindow(WebWindowType)
{
    WebEnginePage *page = new WebEnginePage(this);
    connect(page, &QWebEnginePage::urlChanged, this, &WebEnginePage::onUrlChanged);
    return page;
}

void WebEnginePage::onUrlChanged(const QUrl & url)
{
    if (WebEnginePage *page = qobject_cast<WebEnginePage *>(sender()))
    {
        BrowserWindow::getInstance()->load(url);
        page->deleteLater();
    }
}

BrowserWindow::BrowserWindow(QWidget *parent) :
    QMainWindow(parent)
{    
    // 创建滚动区域
    m_root = new QScrollArea(this);
    m_root->setWidgetResizable(false); // 重要：保持内部部件固定大小
    m_root->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_root->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(m_root);

    setWindowTitle(QString::fromStdWString(L"浏览器"));
    setWindowState(windowState() | Qt::WindowMaximized);
    setProfileName(DEFAULT_PROFILE_NAME);
}

BrowserWindow::~BrowserWindow()
{

}

BrowserWindow* BrowserWindow::getInstance()
{
    static BrowserWindow* instance = new BrowserWindow();
    return instance;
}

void BrowserWindow::setWebViewSize(QSize size)
{
    m_webViewSize = size;
    for (auto it = m_webViews.begin(); it != m_webViews.end(); it++)
    {
        (*it)->resize(size);
    }
}

void BrowserWindow::load(const QUrl& url)
{
    qInfo("load url: %s", url.toString().toStdString().c_str());
    m_webView->load(url);
}

QString BrowserWindow::getUrl()
{
    return m_webView->page()->url().toString();
}

bool BrowserWindow::captrueImage(const QString& savePath)
{
    // 如果最小化就最大化展示，因为最小化时捕获不到图片
    if (isMinimized())
    {
        showMaximized();
    }

    // 截屏
    QImage image(m_webView->size(), QImage::Format_ARGB32);
    QPainter painter(&image);
    m_webView->render(&painter);
    painter.end();
    if (!image.save(savePath))
    {
        qCritical("failed to save the capturing image to %s", savePath.toStdString().c_str());
        return false;
    }
    else
    {
        return true;
    }
}

void BrowserWindow::runJsCode(const QString& id, const QString& jsCode)
{
    m_webView->page()->runJavaScript(jsCode, [this, id](const QVariant &result) {
        emit runJsCodeFinished(id, result);
    });
}

void BrowserWindow::cleanCookie()
{
    QWebEngineProfile *profile = m_webView->page()->profile();

    // Retrieve the cookie store from the profile
    QWebEngineCookieStore *cookieStore = profile->cookieStore();

    // Delete all cookies from the cookie store
    cookieStore->deleteAllCookies();
}

void BrowserWindow::setProfileName(const QString& name)
{
    auto it = m_webViews.find(name);
    if (it == m_webViews.end())
    {
        QWebEngineView* webView = new QWebEngineView(this);
        webView->resize(m_webViewSize);
        if (name != DEFAULT_PROFILE_NAME)
        {
            QWebEngineProfile* profile = new QWebEngineProfile(name, this);
            QWebEnginePage* page = new WebEnginePage(profile, this);
            webView->setPage(page);
        }
        else
        {
            QWebEnginePage* page = new WebEnginePage(this);
            webView->setPage(page);
        }
        webView->page()->profile()->setRequestInterceptor(m_requestInterceptor);
        m_webViews[name] = webView;
    }

    for (auto it = m_webViews.begin(); it != m_webViews.end(); it++)
    {
        if (it.key() == name)
        {
            if (m_webView != *it)
            {
                it.value()->showNormal();
                connect(it.value()->page(), &QWebEnginePage::loadFinished, this, &BrowserWindow::onLoadFinished);
                m_webView = *it;
                m_root->setWidget(m_webView);
            }
        }
        else
        {
            it.value()->hide();
            disconnect(it.value()->page(), &QWebEnginePage::loadFinished, this, &BrowserWindow::onLoadFinished);            
        }
    }
}

void BrowserWindow::setRequestInterceptor(QWebEngineUrlRequestInterceptor* requestInterceptor)
{
    m_requestInterceptor = requestInterceptor;
    for (auto it = m_webViews.begin(); it != m_webViews.end(); it++)
    {
        (*it)->page()->profile()->setRequestInterceptor(requestInterceptor);
    }
}

void BrowserWindow::onLoadFinished(bool ok)
{
    qInfo("finish to load url: %s, result: %s", getUrl().toStdString().c_str(), ok?"success":"failed");
    emit loadFinished(ok);
}

void BrowserWindow::closeEvent(QCloseEvent *event)
{
    if (m_hideWhenClose)
    {
        hide();
        event->ignore();
        return;
    }

    if (m_canClose)
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

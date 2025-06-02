#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QCloseEvent>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QScrollArea>
#include <QNetworkCookie>
#include <QVector>

namespace Ui {
class BrowserWindow;
}

class HttpOnlyCookie
{
public:
    QString m_domain;

    QString m_name;

    QString m_value;
};

class WebEnginePage: public QWebEnginePage
{
    Q_OBJECT

public:
    using QWebEnginePage::QWebEnginePage;

protected:
    virtual QWebEnginePage *createWindow(WebWindowType) override;

private Q_SLOTS:
    void onUrlChanged(const QUrl & url);
};

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

private:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow();

public:
    static BrowserWindow* getInstance();

public:
    // 设置可以关闭退出
    void setCanClose() { m_canClose = true; }

    // 设置webview大小
    void setWebViewSize(QSize size);

    // 加载网页
    void load(const QUrl& url);

    // 获取当前页面url
    QString getUrl();

    // 截图
    bool captrueImage(const QString& savePath);

    // 执行JS脚本
    void runJsCode(const QString& id, const QString& jsCode);

    // 设置关闭窗口是否变成隐藏窗口
    void setHideWhenClose(bool hideWhenClose) { m_hideWhenClose = hideWhenClose;}
    
    // 清除cookie
    void cleanCookie();

    // 设置Profile名字
    void setProfileName(const QString& name);

    // 设置请求拦截器
    void setRequestInterceptor(QWebEngineUrlRequestInterceptor* requestInterceptor);

    // 获取http only cookie
    QString getHttpOnlyCookie(QString domain, QString name);

signals:
    // 加载网页完成
    void loadFinished(bool ok);

    // 执行JS脚本的结果
    void runJsCodeFinished(const QString& id, const QVariant& result);

private slots:
    void onLoadFinished(bool ok);

    void onCookieAdded(const QNetworkCookie &cookie);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // 当前webview
    QWebEngineView* m_webView = nullptr;

    bool m_canClose = false;

    // 关闭窗口是否变成隐藏窗口
    bool m_hideWhenClose = false;

    // WebView大小
    QSize m_webViewSize = QSize(1920, 1080);

    // name -> webview
    QMap<QString, QWebEngineView*> m_webViews;

    QWebEngineUrlRequestInterceptor* m_requestInterceptor = nullptr;

    // 当窗口小的时候，可以通过滚动查看整个网页
    QScrollArea* m_root = nullptr;

    // HttpOnly的Cookies
    QVector<HttpOnlyCookie> m_cookies;
};

#endif // BROWSERWINDOW_H

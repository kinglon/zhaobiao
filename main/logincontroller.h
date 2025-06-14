#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include <QObject>
#include <QTimer>

class LoginController : public QObject
{
    Q_OBJECT
public:
    explicit LoginController(QObject *parent = nullptr);

    // 开始登录
    void run();

private:
    void onTimer();

    // 登录结束
    void stop();

    // 刷新登录
    void startRefreshLoginInfo();

signals:
    void printLog(QString content);

private slots:
    void onRunJsCodeFinished(const QString& id, const QVariant& result);

private:
    bool m_running = false;

    bool m_needFillUserName = false;

    QTimer* m_timer = nullptr;

    // 定时检索，刷新登录token，避免过期
    QTimer* m_refreshTimer = nullptr;
};

#endif // LOGINCONTROLLER_H

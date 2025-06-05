#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include <QObject>
#include <QTimer>

class LoginController : public QObject
{
    Q_OBJECT
public:
    explicit LoginController(QObject *parent = nullptr);

    void run();

private:
    void onTimer();

    void stop();

signals:
    void printLog(QString content);

private slots:
    void onRunJsCodeFinished(const QString& id, const QVariant& result);

private:
    bool m_running = false;

    bool m_needFillUserName = false;

    QTimer* m_timer = nullptr;
};

#endif // LOGINCONTROLLER_H

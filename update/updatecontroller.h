#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <QObject>
#include <QThread>
#include <QVector>

class UpdateThread: public QThread
{
    Q_OBJECT

public:
    explicit UpdateThread(QObject *parent = nullptr);

    void run() override;

private:
    void runInternal();

signals:
    void printLog(QString content);
};

class UpdateController : public QObject
{
    Q_OBJECT
public:
    explicit UpdateController(QObject *parent = nullptr);

    void run();

signals:
    void printLog(QString content);

private slots:
    void onThreadFinish();

private:
    UpdateThread* m_updateThread = nullptr;
};

#endif // UPDATECONTROLLER_H

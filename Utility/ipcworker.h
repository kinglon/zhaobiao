#ifndef IPCWORKER_H
#define IPCWORKER_H

#include <QThread>
#include <QObject>
#include <QSharedMemory>

class IpcWorker : public QThread
{
    Q_OBJECT
public:
    explicit IpcWorker(QObject *parent = nullptr);

public:
    void setKey(QString key) { m_key = key; }

    static bool sendData(QString key, const char* data);

private:
    virtual void run() override;

signals:
    void ipcDataArrive(QString data);

private:
    QString m_key;
};

#endif // IPCWORKER_H

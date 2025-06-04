#include "ipcworker.h"

IpcWorker::IpcWorker(QObject *parent)
    : QThread{parent}
{

}

void IpcWorker::run()
{
    qInfo("ipc worker begin to run");
    if (m_key.isEmpty())
    {
        qCritical("the key is not setted");
        return;
    }

    QSharedMemory sm(m_key);
    if (!sm.create(1024*1024))
    {
        qCritical("failed to create the share memory");
        return;
    }

    // 数据设置为0
    if (!sm.lock())
    {
        qCritical("failed to lock the share memory");
        return;
    }
    memset(sm.data(), 0, 1);
    sm.unlock();

    while (true)
    {
        QThread::msleep(100);
        if (!sm.lock())
        {
            qCritical("failed to lock the share memory");
            QThread::sleep(2);
            continue;
        }
        char* data = (char*)sm.data();
        if (strlen(data) > 0)
        {
            QString dataString = QString::fromUtf8(data);
            qInfo("ipc recv data: %s", data);
            emit ipcDataArrive(dataString);
            memset(data, 0, 1);
        }
        sm.unlock();
    }
}

bool IpcWorker::sendData(QString key, const char* data)
{
    if (key.isEmpty())
    {
        qCritical("the key is not setted");
        return false;
    }

    QSharedMemory sm(key);
    if (!sm.attach())
    {
        qCritical("failed to attach to the share memory with key %s", key.toStdString().c_str());
        return false;
    }

    if (!sm.lock())
    {
        qCritical("failed to lock the share memory");
        return false;
    }

    memcpy(sm.data(), data, strlen(data)+1);
    sm.unlock();

    qInfo("send data: %s", data);

    return true;
}

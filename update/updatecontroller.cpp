#include "updatecontroller.h"
#include "settingmanager.h"
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include "ImPath.h"

#define MAX_FAILED_COUNT 20

UpdateThread::UpdateThread(QObject *parent) : QThread(parent)
{

}

void UpdateThread::run()
{
    qInfo("the updating thread begin");
    runInternal();
    qInfo("the updating thread end");
}

void UpdateThread::runInternal()
{
    //
}

UpdateController::UpdateController(QObject *parent)
    : QObject{parent}
{

}

void UpdateController::run()
{
    if (m_updateThread)
    {
        qInfo("it is updating");
        return;
    }

    m_updateThread = new UpdateThread();
    connect(m_updateThread, &UpdateThread::printLog, this, &UpdateController::printLog);
    connect(m_updateThread, &UpdateThread::finished, this, &UpdateController::onThreadFinish);
    m_updateThread->start();
}

void UpdateController::onThreadFinish()
{    
    m_updateThread->deleteLater();
    m_updateThread = nullptr;
}

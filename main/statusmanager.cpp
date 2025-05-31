#include "statusmanager.h"
#include <QMutexLocker>

StatusManager::StatusManager()
{

}

StatusManager* StatusManager::getInstance()
{
    static StatusManager* instance = new StatusManager();
    return instance;
}

void StatusManager::setCookies(QString cookies)
{
    QMutexLocker locker(&m_mutex);
    m_cookies = cookies;
}

QString StatusManager::getCookies()
{
    QMutexLocker locker(&m_mutex);
    return m_cookies;
}

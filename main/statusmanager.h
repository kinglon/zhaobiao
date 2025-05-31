#ifndef STATUSMANAGER_H
#define STATUSMANAGER_H

#include <QMutex>
#include <QString>

class StatusManager
{
protected:
    StatusManager();

public:
    static StatusManager* getInstance();

    void setCookies(QString cookies);
    QString getCookies();

private:
    QMutex m_mutex;

    QString m_cookies;
};

#endif // STATUSMANAGER_H

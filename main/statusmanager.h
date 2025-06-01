#ifndef STATUSMANAGER_H
#define STATUSMANAGER_H

#include <QMutex>
#include <QString>
#include "settingmanager.h"

class StatusManager
{
protected:
    StatusManager();

public:
    static StatusManager* getInstance();

    void setCookies(QString cookies);
    QString getCookies();

    void setCurrentFilterKeyWord(const FilterKeyWord& filterKeyWord);
    FilterKeyWord getCurrentFilterKeyWord();

private:
    QMutex m_mutex;

    QString m_cookies;

    // 当前搜索的关键词
    FilterKeyWord m_currentFilterKeyWord;
};

#endif // STATUSMANAGER_H

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

public:
    // 需要搜索的关键词列表
    QVector<FilterKeyWord> m_searchFilterKeyWords;

    // 当前搜索的关键词
    FilterKeyWord m_currentFilterKeyWord;

    // 标志是否下载附件
    bool m_downloadAttachment = false;

private:
    QMutex m_mutex;

    QString m_cookies;
};

#endif // STATUSMANAGER_H

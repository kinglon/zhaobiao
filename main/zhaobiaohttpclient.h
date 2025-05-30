#ifndef ZHAOBIAOHTTPCLIENT_H
#define ZHAOBIAOHTTPCLIENT_H

#include <QObject>
#include "Utility/SyncHttpClient.h"
#include "datamodel.h"

class SearchCondition
{
public:
    // 页码，从1开始
    int m_page = 1;

    // 标志是否只查标题
    bool m_onlyTitleField = false;

    // 关键词
    QString m_keyWord;

    // 开始日期，如：2025-05-27
    QString m_beginDate;

    // 结束日期，如：2025-05-27
    QString m_endDate;
};

class ZhaoBiaoHttpClient : public SyncHttpClient
{
    Q_OBJECT

public:
    explicit ZhaoBiaoHttpClient(QObject *parent = nullptr);    

    bool search(const SearchCondition& condition, int& totalPage, QVector<ZhaoBiao>& zhaoBiaos);

    // 方便接口测试
    static void test();

private:
    bool handleSearchReply(QNetworkReply* reply, int& totalPage, QVector<ZhaoBiao>& zhaoBiaos);

public:
    QString m_cookies;

    // 请求失败时，错误信息保存在这里
    QString m_lastError;

    // 请求失败时，标志是否因为未登录导致
    bool m_needLogin = false;
};

#endif // ZHAOBIAOHTTPCLIENT_H

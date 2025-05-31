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

    // 获取详情，包括：正文和附件
    bool getDetail(QString link, ZhaoBiao& zhaoBiao);

    bool downloadFile(QString link, QString savedFilePath);

    // 方便接口测试
    static void test();

private:    
    void handleErrorReply(QNetworkReply* reply);

    bool handleSearchReply(QNetworkReply* reply, int& totalPage, QVector<ZhaoBiao>& zhaoBiaos);

    bool handleGetDetailReply(QNetworkReply* reply, ZhaoBiao& zhaoBiao);

    bool handleDownloadFileReply(QNetworkReply* reply, QString savedFilePath);

public:
    QString m_cookies;

    // 请求失败时，错误信息保存在这里
    QString m_lastError;

    // 请求失败时，标志是否因为未登录导致
    bool m_needLogin = false;

    // 标志是否需要更新cookie
    bool m_needUpdateCookie = false;

    // 更新cookie的js代码，m_needUpdateCookie=true时有效
    QString m_updateCookieJsCode;
};

#endif // ZHAOBIAOHTTPCLIENT_H

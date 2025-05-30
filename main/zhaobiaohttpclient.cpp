#include "zhaobiaohttpclient.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ZhaoBiaoHttpClient::ZhaoBiaoHttpClient(QObject *parent)
    : SyncHttpClient{parent}
{

}

bool ZhaoBiaoHttpClient::search(const SearchCondition& condition, int& totalPage, QVector<ZhaoBiao>& zhaoBiaos)
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://s.zhaobiao.cn/search/async/resultAjax"));

    addCommonHeader(request);
    request.setRawHeader("Cookie", m_cookies.toUtf8());
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("X-Requested-With", "XMLHttpRequest");

    QString body = "searchtype=sj&provinces=&leftday=&pstate=&channels=bidding%2Cfore%2Cchange%2Csucceed%2Cpurchase&attachment=1&searchModel=&associateModel=&correlation=&eliminate=";
    body += QString("&currentpage=%1").arg(condition.m_page);
    QString keyWordEncode = QUrl::toPercentEncoding(condition.m_keyWord);
    body += QString("&queryword=%1").arg(keyWordEncode);
    body += QString("&starttime=%1&endtime=%2").arg(condition.m_beginDate, condition.m_endDate);
    if (condition.m_onlyTitleField)
    {
        body += QString("&field=title");
    }
    else
    {
        body += QString("&field=all");
    }

    QNetworkReply* reply = m_networkAccessManager.post(request, body.toUtf8());

    waitForResponse();

    bool success = handleSearchReply(reply, totalPage, zhaoBiaos);
    reply->deleteLater();

    return success;
}

bool ZhaoBiaoHttpClient::handleSearchReply(QNetworkReply* reply, int& totalPage, QVector<ZhaoBiao>& zhaoBiaos)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        m_lastError = QString::fromWCharArray(L"搜索失败，错误码：%1").arg(reply->error());
        return false;
    }

    QByteArray data;
    if (!getData(reply, data))
    {
        m_lastError = QString::fromWCharArray(L"搜索失败，错误：解压数据遇到问题");
        return false;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        m_lastError = QString::fromWCharArray(L"搜索失败，错误：Json解析失败");
        return false;
    }

    QJsonObject root = jsonDocument.object();
    int status = root["status"].toInt();
    if (status == 300)
    {
        m_needLogin = true;
        m_lastError = QString::fromWCharArray(L"搜索失败，错误：需要登录");
        return false;
    }
    else if (status != 200)
    {
        QString error = root["msg"].toString();
        m_lastError = QString::fromWCharArray(L"搜索失败，错误：%1").arg(error);
        return false;
    }

    totalPage = root["pageInfo"].toObject()["totalPage"].toInt();

    QJsonArray dataList = root["dataList"].toArray();
    for (auto data : dataList)
    {
        QJsonObject dataJsonObject = data.toObject();
        ZhaoBiao zhaoBiao;
        zhaoBiao.m_id = dataJsonObject["docid"].toString();
        zhaoBiao.m_title = dataJsonObject["title"].toString();
        QString proName = dataJsonObject["proName"].toString();
        QStringList parts = proName.split(" ");
        if (parts.length() > 0)
        {
            zhaoBiao.m_province = parts[0].trimmed();
        }
        if (parts.length() > 1)
        {
            zhaoBiao.m_city = parts[1].trimmed();
        }
        zhaoBiao.m_link = dataJsonObject["url"].toString();
        zhaoBiaos.append(zhaoBiao);
    }

    return true;
}

void ZhaoBiaoHttpClient::test()
{
    ZhaoBiaoHttpClient client;
    client.m_cookies = "__jsluid_s=e86731baa7324f3e3f5cb9e3f32ad01f; reg_referer=aHR0cHM6Ly93d3cuemhhb2JpYW8uY24v; _clck=smrqgm%7C2%7Cfwc%7C0%7C1936; cookie_login_domain=www; Cookies_token=340fb233-9289-4aa5-9a19-566f3188417f; JSESSIONID=FA297A5A55DBC1FE04D018D4903F14AC; Cookies_SearchHistory_new=\"MjAyNS0wNS0zMEAjO+efv+WxseW3peeoi+aWveW3pSDnn7/lsbHmlr3lt6Ug55+/5bGx5oC75om/5YyFIOefv+WxseW3peeoi+aAu+aJv+WMhSDnn7/lsbHlt6XnqIvmlr3lt6XmgLvmib/ljIUjIw==\"; _clsk=1uu9lzm%7C1748590906140%7C5%7C0%7Cn.clarity.ms%2Fcollect";

    SearchCondition condition;
    condition.m_keyWord = QString::fromWCharArray(L"矿山工程施工 矿山施工 矿山总承包 矿山工程总承包 矿山工程施工总承包");
    condition.m_beginDate = "2025-05-27";
    condition.m_endDate = "2025-05-30";

    int totalPage = 0;
    QVector<ZhaoBiao> zhaoBiaos;
    client.search(condition, totalPage, zhaoBiaos);
}

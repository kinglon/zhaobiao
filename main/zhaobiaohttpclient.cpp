#include "zhaobiaohttpclient.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <qgumbodocument.h>
#include <qgumbonode.h>
#include <QFile>
#include "settingmanager.h"

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

    QString body = "searchtype=sj&provinces=&leftday=&pstate=&attachment=1&searchModel=&associateModel=&correlation=&eliminate=";
    body += QString("&currentpage=%1").arg(condition.m_page);
    QString keyWordEncode = QUrl::toPercentEncoding(condition.m_keyWord);
    body += QString("&queryword=%1").arg(keyWordEncode);
    body += QString("&channels=%1").arg(condition.m_channels);
    body += QString("&provinces=%1").arg(condition.m_province);
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

bool ZhaoBiaoHttpClient::getDetail(QString link, ZhaoBiao& zhaoBiao)
{
    QNetworkRequest request;
    request.setUrl(QUrl(link));

    addCommonHeader(request);
    request.setRawHeader("Cookie", m_cookies.toUtf8());
    request.setRawHeader("Referer", link.toUtf8());

    QNetworkReply* reply = m_networkAccessManager.get(request);

    waitForResponse();

    bool success = handleGetDetailReply(reply, zhaoBiao);
    reply->deleteLater();

    return success;
}

bool ZhaoBiaoHttpClient::downloadFile(QString link, QString savedFilePath)
{
    QNetworkRequest request;
    request.setUrl(QUrl(link));

    addCommonHeader(request);
    request.setRawHeader("Cookie", m_cookies.toUtf8());

    int oldTimeOut = m_networkAccessManager.transferTimeout();
    m_networkAccessManager.setTransferTimeout(180*1000);
    QNetworkReply* reply = m_networkAccessManager.get(request);

    waitForResponse();

    m_networkAccessManager.setTransferTimeout(oldTimeOut);
    bool success = handleDownloadFileReply(reply, savedFilePath);
    reply->deleteLater();

    return success;
}

void ZhaoBiaoHttpClient::handleErrorReply(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::UnknownServerError)
    {
        QByteArray data;
        if (getData(reply, data))
        {
            QString dataString = QString::fromUtf8(data);
            if (dataString.contains("document.cookie"))
            {
                m_needUpdateCookie = true;
            }
        }
    }
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
        zhaoBiao.m_publishDate = dataJsonObject["ipubtime"].toString();
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

bool ZhaoBiaoHttpClient::handleGetDetailReply(QNetworkReply* reply, ZhaoBiao& zhaoBiao)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        handleErrorReply(reply);
        m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误码：%1").arg(reply->error());
        return false;
    }

    QByteArray data;
    if (!getData(reply, data))
    {
        m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误：解压数据遇到问题");
        return false;
    }

    auto doc = QGumboDocument::parse(data);
    auto root = doc.rootNode();

    // 检查是否登录
    auto notices = root.getElementsByClassName("w-noticeHide");
    if (!notices.empty())
    {
        m_needLogin = true;
        m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误：需要登录");
        return false;
    }

    auto details = root.getElementsByClassName("bid_details_zw");
    if (details.empty())
    {
        m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误：没有解析到正文内容的元素");
        return false;
    }

    zhaoBiao.m_content = details[0].innerTextV2();

    auto accessories = root.getElementsByClassName("accessory_item");
    zhaoBiao.m_attachments.clear();
    for (auto& accessory : accessories)
    {
        Attachment attachment;
        auto fileNameNodes = accessory.getElementsByClassName("i-left-text");
        if (fileNameNodes.empty())
        {
            m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误：没有解析到附件的文件名元素");
            return false;
        }

        auto part1Nodes = fileNameNodes[0].getElementsByClassName("ellipsis");
        if (part1Nodes.empty())
        {
            m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误：没有解析到附件的文件名部分");
            return false;
        }
        attachment.m_fileName = part1Nodes[0].innerTextV2().trimmed();

        auto part2Nodes = fileNameNodes[0].getElementsByClassName("tuning");
        if (!part2Nodes.empty())
        {
            attachment.m_fileName += QString(".") + part2Nodes[0].innerTextV2().trimmed();
        }

        auto downloadUrls = accessory.getElementsByTagName(HtmlTag::A);
        if (downloadUrls.empty())
        {
            m_lastError = QString::fromWCharArray(L"获取正文内容和附件失败，错误：没有解析到附件的下载地址");
            return false;
        }
        attachment.m_downloadLink = downloadUrls[0].getAttribute("href");
        zhaoBiao.m_attachments.append(attachment);
    }

    return true;
}

bool ZhaoBiaoHttpClient::handleDownloadFileReply(QNetworkReply* reply, QString savedFilePath)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        m_lastError = QString::fromWCharArray(L"下载文件失败，错误码：%1").arg(reply->error());
        return false;
    }

    QByteArray data;
    if (!getData(reply, data))
    {
        m_lastError = QString::fromWCharArray(L"下载文件失败，错误：解压数据遇到问题");
        return false;
    }

    if (QString::fromUtf8(data).contains("请登录后重试"))
    {
        m_needLogin = true;
        m_lastError = QString::fromWCharArray(L"下载文件失败，错误：未登录");
        return false;
    }

    QFile file(savedFilePath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(data);
        file.close();
        return true;
    }
    else
    {
        m_lastError = QString::fromWCharArray(L"下载文件失败，错误：写入数据到文件遇到问题");
        return false;
    }
}

void ZhaoBiaoHttpClient::test()
{
    ZhaoBiaoHttpClient client;
    client.m_cookies = "__jsluid_s=d48f337061c68a7b9199aa374d4fe592; reg_referer=aHR0cHM6Ly93d3cuemhhb2JpYW8uY24v; cookie_login_domain=www; _clck=smrqgm%7C2%7Cfwd%7C0%7C1936; Cookies_token=1532ab00-5839-4138-a877-4383b87073a5; _clsk=xo095b%7C1748687878578%7C8%7C0%7Cn.clarity.ms%2Fcollect";

//    // 搜索
//    SearchCondition condition;
//    condition.m_keyWord = QString::fromWCharArray(L"矿山工程施工 矿山施工 矿山总承包 矿山工程总承包 矿山工程施工总承包");
//    condition.m_beginDate = "2025-05-27";
//    condition.m_endDate = "2025-05-30";

//    int totalPage = 0;
//    QVector<ZhaoBiao> zhaoBiaos;
//    client.search(condition, totalPage, zhaoBiaos);

    // 获取正文内容和附件
    ZhaoBiao zhaoBiao;
    client.getDetail("https://zb.zhaobiao.cn/bidding_v_320693088677f9e9ec35de20c0bc3690.html", zhaoBiao);

//    // 下载文件
//    QString downloadUrl = "https://zbfile.zhaobiao.cn/resources/styles/v2/jsp/bidFiledown.jsp?id=2093062696&provCode=511700&channel=bidding&docid=197120455&user=42994fb59c456d70d9e7730e1b4e6833";
//    client.downloadFile(downloadUrl, R"(C:\Users\zengxiangbin\Downloads\aaa.pdf)");
}

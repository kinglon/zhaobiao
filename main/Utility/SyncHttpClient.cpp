#include "SyncHttpClient.h"
#include <QNetworkProxy>
#include <QNetworkReply>
#include "qcompressor.h"

SyncHttpClient::SyncHttpClient(QObject *parent)
    : QObject{parent}
{
    m_networkAccessManager.setProxy(QNetworkProxy());
    m_networkAccessManager.setTransferTimeout(10*1000);
    connect(&m_networkAccessManager, &QNetworkAccessManager::finished, this, &SyncHttpClient::onHttpFinished);
}

void SyncHttpClient::onHttpFinished(QNetworkReply *reply)
{    
    (void)reply;
    if (m_eventLoop)
    {
        m_eventLoop->quit();
    }
}

void SyncHttpClient::addCommonHeader(QNetworkRequest& request)
{
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) QtWebEngine/5.15.2 Chrome/83.0.4103.122 Safari/537.36");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept-Encoding", "gzip");
}

void SyncHttpClient::waitForResponse()
{
    m_eventLoop = new QEventLoop();
    m_eventLoop->exec();
    m_eventLoop->deleteLater();
    m_eventLoop = nullptr;
}

bool SyncHttpClient::getData(QNetworkReply* reply, QByteArray& data)
{
    // 获取压缩格式
    QString contentEncoding;
    QList<QByteArray> headers = reply->rawHeaderList();
    for (const QByteArray &header : headers)
    {
        if (header.toLower() == "content-encoding")
        {
            contentEncoding = reply->rawHeader(header).toLower();
            break;
        }
    }

    data = reply->readAll();
    if (contentEncoding == "gzip")
    {
        QByteArray resultData;
        if (!QCompressor::gzipDecompress(data, resultData))
        {
            qCritical("failed to decompress the gzip response data");
            return false;
        }
        else
        {
            data = resultData;
            return true;
        }
    }

    return true;
}

void SyncHttpClient::printRequest(QNetworkRequest& request)
{
    qDebug("url: %s", request.url().toString().toStdString().c_str());
    auto headers = request.rawHeaderList();
    for (auto& header : headers)
    {
        qDebug("header %s: %s", header.toStdString().c_str(), request.rawHeader(header).toStdString().c_str());
    }
}

#ifndef SYNCHTTPCLIENT_H
#define SYNCHTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

class SyncHttpClient : public QObject
{
    Q_OBJECT
public:
    explicit SyncHttpClient(QObject *parent = nullptr);

public:
    void setTransferTimeout(int timeOutMs) { m_networkAccessManager.setTransferTimeout(timeOutMs); }

protected:
    void addCommonHeader(QNetworkRequest& request);

    void waitForResponse();

    // 获取响应数据，如果有压缩会自动解压缩
    bool getData(QNetworkReply* reply, QByteArray& data);

    // 打印请求信息，方便排查问题
    void printRequest(QNetworkRequest& request);

private slots:
    void onHttpFinished(QNetworkReply *reply);

protected:
    QNetworkAccessManager m_networkAccessManager;

    QEventLoop* m_eventLoop = nullptr;
};

#endif // SYNCHTTPCLIENT_H

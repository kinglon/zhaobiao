#ifndef COLLECTCONTROLLER_H
#define COLLECTCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QVector>
#include "zhaobiaohttpclient.h"
#include "datamodel.h"

class CollectThread: public QThread
{
    Q_OBJECT

public:
    explicit CollectThread(QObject *parent = nullptr);

    void run() override;

private:
    void runInternal();

    bool search(ZhaoBiaoHttpClient& client, SearchCondition condition, QVector<ZhaoBiao>& zhaoBiaos);

    // 去重追加
    void appendZhaoBiao(QVector<ZhaoBiao>& zhaoBiaos, const QVector<ZhaoBiao>& newZhaoBiaos);

    void handleZhaoBiaoClientError(ZhaoBiaoHttpClient& client, ZhaoBiao* currentZhaoBiao);

    bool doGetDetail(ZhaoBiaoHttpClient& client, QVector<ZhaoBiao>& zhaoBiaos);

    // 对项目重要性进行分类
    void doPriority(QVector<ZhaoBiao>& zhaoBiaos);

    bool doSave(const QVector<ZhaoBiao>& zhaoBiaos);

    // 下载附件
    bool doDownload(ZhaoBiaoHttpClient& client, const QVector<ZhaoBiao>& zhaoBiaos);

signals:
    void printLog(QString content);

    // link项目链接，title项目标题
    void updateCookie(QString link, QString title);

public:
    std::atomic<bool> m_exit = false;

    bool m_success = false;

    // 采集数据保存的目录
    QString m_savedPath;
};

class CollectController : public QObject
{
    Q_OBJECT
public:
    explicit CollectController(QObject *parent = nullptr);

    void run();

    void stop();

signals:
    void printLog(QString content);

    void runFinish(bool success, QString savedPath);

private slots:
    void onThreadFinish();

    // link项目链接，title项目标题
    void onUpdateCookie(QString link, QString title);

    void onRunJsCodeFinished(const QString& id, const QVariant& result);

private:
    CollectThread* m_collectThread = nullptr;

    QTimer* m_updateCookieTimer = nullptr;
};

#endif // COLLECTCONTROLLER_H

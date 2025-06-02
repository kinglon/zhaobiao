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

    void handleZhaoBiaoClientError(ZhaoBiaoHttpClient& client);

    bool doGetDetail(ZhaoBiaoHttpClient& client, QVector<ZhaoBiao>& zhaoBiaos);

    // 对项目重要性进行分类
    void doPriority(QVector<ZhaoBiao>& zhaoBiaos);

    bool doSave(const QVector<ZhaoBiao>& zhaoBiaos);

    // 下载附件
    bool doDownload(ZhaoBiaoHttpClient& client, const QVector<ZhaoBiao>& zhaoBiaos);

signals:
    void printLog(QString content);

    void updateCookie(QString jsCode);

public:
    std::atomic<bool> m_exit = false;

    bool m_success = false;

    // 采集数据保存的目录
    QString m_savedPath;

private:
    // 用于刷新cookie的链接
    QString m_linkForUpdatingCookie;
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

    void onUpdateCookie(QString link);

    void onRunJsCodeFinished(const QString& id, const QVariant& result);

    void onLoadFinished(bool ok);

private:
    CollectThread* m_collectThread = nullptr;
};

#endif // COLLECTCONTROLLER_H

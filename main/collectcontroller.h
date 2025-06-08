#ifndef COLLECTCONTROLLER_H
#define COLLECTCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QVector>
#include "zhaobiaohttpclient.h"
#include "datamodel.h"

// 采集线程基类
class CollectThread: public QThread
{
    Q_OBJECT

public:
    explicit CollectThread(QObject *parent = nullptr);    

protected:
    void run() override;

    // 实现采集流程
    virtual void runInternal() = 0;

    bool search(ZhaoBiaoHttpClient& client, SearchCondition condition, QVector<ZhaoBiao>& zhaoBiaos);

    // 去重追加
    void appendZhaoBiao(QVector<ZhaoBiao>& zhaoBiaos, const QVector<ZhaoBiao>& newZhaoBiaos);

    void handleZhaoBiaoClientError(ZhaoBiaoHttpClient& client, ZhaoBiao* currentZhaoBiao);

    bool doGetDetail(ZhaoBiaoHttpClient& client, QVector<ZhaoBiao>& zhaoBiaos);    

    bool doSave(QString excelFilePath, const QVector<ZhaoBiao>& zhaoBiaos);

    // 下载附件
    bool doDownload(ZhaoBiaoHttpClient& client, const QVector<ZhaoBiao>& zhaoBiaos);

signals:
    void printLog(QString content);

    // link项目链接，title项目标题
    void updateCookie(QString link, QString title);

public:
    std::atomic<bool> m_exit = false;

    bool m_success = false;

    // 采集数据保存的目录，尾部不带有后划线
    QString m_savedPath;
};

// 矿山类的采集流程
class KuangShanCollectThread : public CollectThread
{
    Q_OBJECT

protected:
    virtual void runInternal() override;

private:
    bool doSave(const QVector<ZhaoBiao>& zhaoBiaos);
};

// 爆破服务类的采集流程
class BaoPoFuWuCollectThread : public CollectThread
{
    Q_OBJECT

protected:
    virtual void runInternal() override;

private:
    bool doSave(const QVector<ZhaoBiao>& zhaoBiaos);
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
    void doCollectNextKeyWords();

    void doFinish(bool success);

private:
    // 标志是否正在运行
    bool m_running = false;

    // 标志是否请求退出
    bool m_requestStop = false;

    // 当前采集的线程
    CollectThread* m_collectThread = nullptr;

    QTimer* m_updateCookieTimer = nullptr;

    // 采集结果保存根路径，尾部没有后划线
    QString m_savedRootPath;

    // 下一个采集的关键词索引
    int m_nextKeyWordIndex = 0;
};

#endif // COLLECTCONTROLLER_H

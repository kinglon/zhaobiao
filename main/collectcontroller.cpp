#include "collectcontroller.h"
#include "zhaobiaohttpclient.h"
#include "statusmanager.h"
#include "settingmanager.h"
#include "browserwindow.h"
#include <QDateTime>
#include <QDir>
#include "Utility/ImPath.h"
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace QXlsx;

#define MAX_FAILED_COUNT 20

CollectThread::CollectThread(QObject *parent) : QThread(parent)
{
    //
}

void CollectThread::run()
{
    qInfo("the collecting thread begin");

    runInternal();
    if (m_success)
    {
        emit printLog(QString::fromWCharArray(L"采集完成"));
    }
    else
    {
        emit printLog(QString::fromWCharArray(L"采集失败"));
    }

    qInfo("the collecting thread end");
}

void CollectThread::runInternal()
{
    ZhaoBiaoHttpClient client;
    client.m_cookies = StatusManager::getInstance()->getCookies();

    // 获取今天和三天前的日期
    QDate currentDate = QDate::currentDate();
    QString currentDateStr = currentDate.toString("yyyy-MM-dd");
    QDate threeDaysAgo = currentDate.addDays(-3); // 减去 3 天
    QString threeDaysAgoStr = threeDaysAgo.toString("yyyy-MM-dd");

    // 按标题关键词搜索
    emit printLog(QString::fromWCharArray(L"按标题关键词搜索"));

    SearchCondition condition;
    condition.m_beginDate = threeDaysAgoStr;
    condition.m_endDate = currentDateStr;
    condition.m_onlyTitleField = true;
    condition.m_keyWord = StatusManager::getInstance()->getCurrentFilterKeyWord().m_titleKeyWord;

    QVector<ZhaoBiao> zhaoBiaosByTitle;
    if (!search(client, condition, zhaoBiaosByTitle))
    {
        return;
    }

    emit printLog(QString::fromWCharArray(L"按标题关键词搜索到%1条").arg(zhaoBiaosByTitle.length()));

    // 按内容附件关键词搜索
    emit printLog(QString::fromWCharArray(L"按内容附件关键词搜索"));

    condition.m_onlyTitleField = false;
    condition.m_keyWord = StatusManager::getInstance()->getCurrentFilterKeyWord().m_contentKeyWord;

    QVector<ZhaoBiao> zhaoBiaosByContent;
    if (!search(client, condition, zhaoBiaosByContent))
    {
        return;
    }

    emit printLog(QString::fromWCharArray(L"按内容附件关键词搜索到%1条").arg(zhaoBiaosByContent.length()));

    // 筛选既含标题关键词又含内容附件关键词的项目    
    QVector<ZhaoBiao> targetZhaoBiaos;
    for (const auto& zhaoBiaoByTitle : zhaoBiaosByTitle)
    {
        for (const auto& zhaoBiaoByContent : zhaoBiaosByContent)
        {
            if (zhaoBiaoByTitle.m_id == zhaoBiaoByContent.m_id)
            {
                targetZhaoBiaos.append(zhaoBiaoByTitle);
                break;
            }
        }
    }

    emit printLog(QString::fromWCharArray(L"筛选既含标题关键词又含内容附件关键词：%1条").arg(targetZhaoBiaos.length()));

    // 对项目需要和不需要进行分类
    int notNeedCount = 0;
    for (auto& zhaoBiao : targetZhaoBiaos)
    {
        for (const auto& excludeKeyWord : SettingManager::getInstance()->m_excludeKeyWords)
        {
            if (zhaoBiao.m_title.contains(excludeKeyWord) || zhaoBiao.m_content.contains(excludeKeyWord))
            {
                zhaoBiao.m_filterResult = ZhaoBiao::FILTER_RESULT_NOT_NEED;
                notNeedCount += 1;
                break;
            }
        }
    }

    emit printLog(QString::fromWCharArray(L"对项目需要和不需要进行分类，不需要%1条，需要%2条").arg(
                      QString::number(notNeedCount),
                      QString::number(targetZhaoBiaos.length()-notNeedCount)));

    // 对项目重要性进行分类
    doPriority(targetZhaoBiaos);

    // 保存采集结果
    emit printLog(QString::fromWCharArray(L"保存采集结果"));
    if (!doSave(targetZhaoBiaos))
    {
        return;
    }

    // 下载重要项目的附件
    emit printLog(QString::fromWCharArray(L"下载重要项目的附件"));
    doDownload(client, targetZhaoBiaos);

    m_success = true;
}

bool CollectThread::search(ZhaoBiaoHttpClient& client, SearchCondition condition, QVector<ZhaoBiao>& resultZhaoBiaos)
{
    int currentPage = 1;
    int failedCount = 0;
    while (true)
    {
        if (failedCount >= MAX_FAILED_COUNT)
        {
            emit printLog(QString::fromWCharArray(L"失败已达到最大次数"));
            return false;
        }

        if (m_exit.load())
        {
            return false;
        }

        QThread::sleep(1);
        emit printLog(QString::fromWCharArray(L"获取第%1页的项目").arg(currentPage));

        condition.m_page = currentPage;

        QVector<ZhaoBiao> zhaoBiaos;
        int totalPage = 0;
        if (client.search(condition, totalPage, zhaoBiaos))
        {
            appendZhaoBiao(resultZhaoBiaos, zhaoBiaos);
            if (currentPage >= totalPage)
            {
                return true;
            }
            currentPage += 1;
            failedCount = 0;
        }
        else
        {
            failedCount += 1;
            handleZhaoBiaoClientError(client);
        }
    }

    return false;
}

void CollectThread::appendZhaoBiao(QVector<ZhaoBiao>& zhaoBiaos, const QVector<ZhaoBiao>& newZhaoBiaos)
{
    for (const auto& newZhaoBiao : newZhaoBiaos)
    {
        bool found = false;
        for (const auto& zhaoBiao : zhaoBiaos)
        {
            if (newZhaoBiao.m_id == zhaoBiao.m_id)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            zhaoBiaos.append(newZhaoBiao);
        }
    }
}

void CollectThread::handleZhaoBiaoClientError(ZhaoBiaoHttpClient& client)
{
    if (client.m_needLogin || client.m_needUpdateCookie)
    {
        StatusManager::getInstance()->setCookies("");
        for (int i=0; i<60000000; i++)
        {
            if (i%5==0)
            {
                if (client.m_needLogin)
                {
                    emit printLog(QString::fromWCharArray(L"请手动登录"));
                }
                else if (client.m_needUpdateCookie)
                {
                    emit printLog(QString::fromWCharArray(L"正在刷新Cookie"));
                    QString jsCode = client.m_updateCookieJsCode + "\n" + "document.cookie";
                    emit updateCookie(jsCode);
                }
            }

            if (m_exit.load())
            {
                return;
            }

            QThread::sleep(1);

            QString cookies = StatusManager::getInstance()->getCookies();
            if (!cookies.isEmpty())
            {
                client.m_needLogin = false;
                client.m_needUpdateCookie = false;
                client.m_updateCookieJsCode = "";
                client.m_cookies = cookies;
                return;
            }
        }
    }

    emit printLog(client.m_lastError);
}

void CollectThread::doPriority(QVector<ZhaoBiao>& zhaoBiaos)
{
    int highCount = 0;
    int mediumCount = 0;
    for (auto& zhaoBiao : zhaoBiaos)
    {
        if (SettingManager::getInstance()->m_regions.contains(zhaoBiao.m_province))
        {
            bool high = false;
            if (zhaoBiao.m_content.contains(QString::fromWCharArray(L"矿山总承包施工")))
            {
                high = true;
            }
            else if (zhaoBiao.m_content.contains(QString::fromWCharArray(L"矿山总承包施工二级"))
                    && zhaoBiao.m_content.contains(QString::fromWCharArray(L"爆破作业单位许可证")))
            {
                high = true;
            }

            if (high)
            {
                zhaoBiao.m_priorityLevel = ZhaoBiao::PRIORITY_LEVEL_HIGH;
                highCount += 1;
                continue;
            }
        }

        if (zhaoBiao.m_content.contains(QString::fromWCharArray(L"矿山总承包施工"))
                || zhaoBiao.m_content.contains(QString::fromWCharArray(L"地质灾害防治工程施工")))
        {
            zhaoBiao.m_priorityLevel = ZhaoBiao::PRIORITY_LEVEL_MEDIUM;
            mediumCount += 1;
            continue;
        }
    }

    emit printLog(QString::fromWCharArray(L"对项目重要性进行分类，重要级%1个，次重要级%2个，一般级%3个").arg(
                      QString::number(highCount), QString::number(mediumCount),
                      QString::number(zhaoBiaos.length()-highCount-mediumCount)));
}

bool CollectThread::doSave(const QVector<ZhaoBiao>& zhaoBiaos)
{
    // 创建保存目录
    m_savedPath = QString::fromStdWString(CImPath::GetDataPath() + L"collect\\");
    m_savedPath += QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    if (!QDir().mkpath(m_savedPath))
    {
        emit printLog(QString::fromWCharArray(L"创建保存目录失败：%1").arg(m_savedPath));
        return false;
    }

    // 拷贝表格模板到保存目录
    QString excelFileName = QString::fromWCharArray(L"输出表格模板.xlsx");
    QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + excelFileName;
    QString destExcelFilePath = m_savedPath+"\\"+QString::fromWCharArray(L"招标项目.xlsx");
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        emit printLog(QString::fromWCharArray(L"拷贝表格模板到保存目录失败"));
        return false;
    }

    Document xlsx(destExcelFilePath);
    if (!xlsx.load())
    {
        emit printLog(QString::fromWCharArray(L"打开输出表格失败"));
        return false;
    }

    // 写入关键词
    FilterKeyWord keyWord = StatusManager::getInstance()->getCurrentFilterKeyWord();
    xlsx.write(1, 2, keyWord.m_titleKeyWord+" "+keyWord.m_contentKeyWord);

    for (int i=0; i<zhaoBiaos.length(); i++)
    {
        int row = i+3;
        int column = 1;
        xlsx.write(row, column++, QString::number(i+1));
        xlsx.write(row, column++, zhaoBiaos[i].m_title);
        xlsx.write(row, column++, QString::number(zhaoBiaos[i].m_filterResult));
        xlsx.write(row, column++, zhaoBiaos[i].getPriorityLevel());
        xlsx.write(row, column++, zhaoBiaos[i].m_province+zhaoBiaos[i].m_city);
        xlsx.write(row, column++, zhaoBiaos[i].m_link);
        row++;
    }

    if (!xlsx.save())
    {
        emit printLog(QString::fromWCharArray(L"保存输出表格失败"));
        return false;
    }

    // 写入正文内容
    for (int i=0; i<zhaoBiaos.length(); i++)
    {
        QString currentSavePath = m_savedPath+"\\"+QString::number(i+1);
        QDir().mkpath(currentSavePath);
        QString contentFilePath = currentSavePath+"\\"+QString::fromWCharArray(L"正文.txt");
        QFile file(contentFilePath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(zhaoBiaos[i].m_content.toUtf8());
            file.close();
        }
    }

    return true;
}

bool CollectThread::doDownload(ZhaoBiaoHttpClient& client, const QVector<ZhaoBiao>& zhaoBiaos)
{
    int total = 0;
    for (const auto& zhaoBiao : zhaoBiaos)
    {
        if (zhaoBiao.m_priorityLevel == ZhaoBiao::PRIORITY_LEVEL_HIGH)
        {
            total += zhaoBiao.m_attachments.length();
        }
    }

    if (total == 0)
    {
        emit printLog(QString::fromWCharArray(L"没有附件需要下载"));
        return true;
    }

    emit printLog(QString::fromWCharArray(L"下载附件，共%1个").arg(total));

    int count = 0;
    for (int zhaoBiaoIndex=0; zhaoBiaoIndex<zhaoBiaos.length(); zhaoBiaoIndex++)
    {
        const auto& zhaoBiao = zhaoBiaos[zhaoBiaoIndex];
        if (m_exit.load())
        {
            return false;
        }

        if (zhaoBiao.m_priorityLevel != ZhaoBiao::PRIORITY_LEVEL_HIGH)
        {
            continue;
        }

        for (const auto& attachment : zhaoBiao.m_attachments)
        {
            if (m_exit.load())
            {
                return false;
            }

            // 重试3次
            for (int i=0; i<3; i++)
            {
                if (m_exit.load())
                {
                    return false;
                }

                QString currentSavePath = m_savedPath+"\\"+QString::number(zhaoBiaoIndex+1);
                QDir().mkpath(currentSavePath);
                QString contentFilePath = currentSavePath+"\\"+attachment.m_fileName;
                if (!client.downloadFile(attachment.m_downloadLink, contentFilePath))
                {
                    emit printLog(client.m_lastError);
                }
                else
                {
                    count++;
                    emit printLog(QString::fromWCharArray(L"下载附件进度：%1/%2").arg(QString::number(count), QString::number(total)));
                    break;
                }
            }
        }
    }

    if (count == total)
    {
        emit printLog(QString::fromWCharArray(L"下载附件完成"));
        return true;
    }

    return false;
}

CollectController::CollectController(QObject *parent)
    : QObject{parent}
{

}

void CollectController::run()
{
    if (m_collectThread)
    {
        qInfo("it is collecting");
        return;
    }

    m_collectThread = new CollectThread();
    connect(m_collectThread, &CollectThread::printLog, this, &CollectController::printLog);
    connect(m_collectThread, &CollectThread::updateCookie, this, &CollectController::onUpdateCookie);
    connect(m_collectThread, &CollectThread::finished, this, &CollectController::onThreadFinish);
    m_collectThread->start();

    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCode, this, &CollectController::onRunJsCodeFinished);
}

void CollectController::stop()
{
    if (m_collectThread)
    {
        m_collectThread->m_exit.store(true);
    }
}

void CollectController::onThreadFinish()
{
    disconnect(BrowserWindow::getInstance(), nullptr, this, nullptr);

    bool success = m_collectThread->m_success;
    QString savedPath = m_collectThread->m_savedPath;

    m_collectThread->deleteLater();
    m_collectThread = nullptr;

    emit runFinish(success, savedPath);
}

void CollectController::onUpdateCookie(QString jsCode)
{
    BrowserWindow::getInstance()->runJsCode("UpdateCookie", jsCode);
}

void CollectController::onRunJsCodeFinished(const QString& id, const QVariant& result)
{
    if (id != "UpdateCookie")
    {
        return;
    }

    QString cookie = result.toString();
    if (cookie.isEmpty())
    {
        qInfo("cookie: %s", cookie.toStdString().c_str());
        StatusManager::getInstance()->setCookies(cookie);
    }
}

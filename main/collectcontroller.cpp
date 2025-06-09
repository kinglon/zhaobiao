#include "collectcontroller.h"
#include "zhaobiaohttpclient.h"
#include "statusmanager.h"
#include "settingmanager.h"
#include "browserwindow.h"
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include "Utility/ImPath.h"
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
#include "jscodemanager.h"

using namespace QXlsx;

#define MAX_FAILED_COUNT 20

CollectThread::CollectThread(QObject *parent) : QThread(parent)
{

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

            // 开发人员测试，只采集前10页
            if (SettingManager::getInstance()->m_debug && currentPage >= 10)
            {
                emit printLog(QString::fromWCharArray(L"开发人员测试，只采集前10页"));
                return true;
            }

            currentPage += 1;
            failedCount = 0;
        }
        else
        {
            failedCount += 1;
            handleZhaoBiaoClientError(client, nullptr);
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

void CollectThread::handleZhaoBiaoClientError(ZhaoBiaoHttpClient& client, ZhaoBiao* currentZhaoBiao)
{
    if (client.m_needLogin || client.m_needUpdateCookie)
    {
        StatusManager::getInstance()->setCookies("");
        for (int i=0; i<60000000; i++)
        {
            int interval = 60;
            if (SettingManager::getInstance()->m_debug)
            {
                interval = 1000;
            }
            if (i % interval == 0)
            {
                if (client.m_needLogin)
                {
                    emit printLog(QString::fromWCharArray(L"请手动登录"));
                }
                else if (client.m_needUpdateCookie)
                {
                    if (currentZhaoBiao == nullptr)
                    {
                        return;
                    }

                    emit printLog(QString::fromWCharArray(L"正在刷新Cookie"));
                    emit updateCookie(currentZhaoBiao->m_link, currentZhaoBiao->m_title);
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
                client.m_cookies = cookies;
                return;
            }
        }
    }

    emit printLog(client.m_lastError);
}

bool CollectThread::doGetDetail(ZhaoBiaoHttpClient& client, QVector<ZhaoBiao>& zhaoBiaos)
{
    // 统计重要项目的个数
    int total = 0;
    for (const auto& zhaoBiao : zhaoBiaos)
    {
        if (zhaoBiao.m_priorityLevel == ZhaoBiao::PRIORITY_LEVEL_HIGH)
        {
            total += 1;
        }
    }

    if (total == 0)
    {
        emit printLog(QString::fromWCharArray(L"没有重要项目的详情需要获取"));
        return true;
    }

    for (int i=0; i<zhaoBiaos.length(); i++)
    {
        ZhaoBiao& zhaoBiao = zhaoBiaos[i];
        if (zhaoBiao.m_priorityLevel != ZhaoBiao::PRIORITY_LEVEL_HIGH)
        {
            continue;
        }

        if (m_exit.load())
        {
            return false;
        }

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

            ZhaoBiao zhaoBiaoDetail;
            if (client.getDetail(zhaoBiao.m_link, zhaoBiaoDetail))
            {
                zhaoBiao.m_content = zhaoBiaoDetail.m_content;
                zhaoBiao.m_attachments = zhaoBiaoDetail.m_attachments;
                break;
            }
            else
            {
                failedCount += 1;
                handleZhaoBiaoClientError(client, &zhaoBiao);
            }
        }

        emit printLog(QString::fromWCharArray(L"获取重要项目的详情：%1/%2").arg(QString::number(i+1), QString::number(total)));
    }

    return true;
}

bool CollectThread::doSave(QString excelFilePath, const QVector<ZhaoBiao>& zhaoBiaos)
{
    Document xlsx(excelFilePath);
    if (!xlsx.load())
    {
        emit printLog(QString::fromWCharArray(L"打开输出表格失败"));
        return false;
    }

    // 写入关键词
    FilterKeyWord keyWord = StatusManager::getInstance()->m_currentFilterKeyWord;
    xlsx.write(1, 2, keyWord.m_titleKeyWord+" "+keyWord.m_contentKeyWord);

    for (int i=0; i<zhaoBiaos.length(); i++)
    {
        int row = i+3;
        int column = 1;
        xlsx.write(row, column++, zhaoBiaos[i].m_id);
        xlsx.write(row, column++, zhaoBiaos[i].m_title);
        xlsx.write(row, column++, zhaoBiaos[i].m_publishDate);
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
        emit printLog(QString::fromWCharArray(L"重要级项目没有附件需要下载"));
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

        for (int i=0; i<zhaoBiao.m_attachments.length(); i++)
        {
            if (m_exit.load())
            {
                return false;
            }

            const auto& attachment = zhaoBiao.m_attachments[i];

            // 重试3次
            bool ok = false;
            for (int i=0; i<3; i++)
            {
                if (m_exit.load())
                {
                    return false;
                }

                QString currentSavePath = m_savedPath+"\\"+zhaoBiao.m_id;
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
                    ok = true;
                    break;
                }
            }

            if (!ok)
            {
                emit printLog(QString::fromWCharArray(L"下载附件失败，项目：%1，第%2个附件").arg(zhaoBiao.m_title, QString::number(i+1)));
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

void BaoPoFuWuCollectThread::runInternal()
{
    ZhaoBiaoHttpClient client;
    client.m_cookies = StatusManager::getInstance()->getCookies();

    QDate searchBeginDate = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchBeginDate).date();
    QString searchBeginDateStr = searchBeginDate.toString("yyyy-MM-dd");
    QDate searchEndDate = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchEndDate).date();
    QString searchEndDateStr = searchEndDate.toString("yyyy-MM-dd");
    qInfo("search date from %s to %s", searchBeginDateStr.toStdString().c_str(), searchEndDateStr.toStdString().c_str());

    // 按内容附件关键词搜索
    emit printLog(QString::fromWCharArray(L"按内容附件关键词搜索"));

    SearchCondition condition;
    condition.m_beginDate = searchBeginDateStr;
    condition.m_endDate = searchEndDateStr;
    condition.m_onlyTitleField = false;
    condition.m_province = SettingManager::getInstance()->m_searchProvince;
    condition.m_keyWord = StatusManager::getInstance()->m_currentFilterKeyWord.m_contentKeyWord;
    condition.m_channels = "bidding%2Cfore%2Cpurchase%2Cfree%2Crecommend%2Cproposed%2Centrust%2Capproved%2Ccommerce%2Clisted%2Cproperty%2Cmineral%2Cland%2Cauction%2Cother";

    QVector<ZhaoBiao> zhaoBiaosByContent;
    if (!search(client, condition, zhaoBiaosByContent))
    {
        return;
    }

    emit printLog(QString::fromWCharArray(L"按内容附件关键词搜索到：%1条").arg(zhaoBiaosByContent.length()));

    // 筛选含有标题关键词项目
    QStringList titleKeywords = StatusManager::getInstance()->m_currentFilterKeyWord.m_titleKeyWord.split(" ");
    QVector<ZhaoBiao> targetZhaoBiaos;
    for (const auto& zhaoBiaoByContent : zhaoBiaosByContent)
    {
        for (const auto& titleKeyWord : titleKeywords)
        {
            if (zhaoBiaoByContent.m_title.contains(titleKeyWord))
            {
                targetZhaoBiaos.append(zhaoBiaoByContent);
                break;
            }
        }
    }

    emit printLog(QString::fromWCharArray(L"筛选含有标题关键词项目：%1条").arg(targetZhaoBiaos.length()));

    // 筛选掉标题不含关键词项目
    QString excludeKeyWord = StatusManager::getInstance()->m_currentFilterKeyWord.m_titleExcludeKeyWord;
    if (!excludeKeyWord.isEmpty())
    {
        QStringList excludeTitleKeywords = excludeKeyWord.split(" ");
        QVector<ZhaoBiao> targetZhaoBiaos2;
        for (const auto& zhaoBiao : targetZhaoBiaos)
        {
            bool found = false;
            for (const auto& keyWord : excludeTitleKeywords)
            {
                if (zhaoBiao.m_title.contains(keyWord))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                targetZhaoBiaos2.append(zhaoBiao);
            }
        }

        targetZhaoBiaos = targetZhaoBiaos2;
        emit printLog(QString::fromWCharArray(L"筛选掉标题不含关键词项目后剩余：%1条").arg(targetZhaoBiaos.length()));
    }

    if (targetZhaoBiaos.length() == 0)
    {
        m_success = true;
        return;
    }

    // 保存采集结果
    emit printLog(QString::fromWCharArray(L"保存采集结果"));
    if (!doSave(targetZhaoBiaos))
    {
        return;
    }

    if (!StatusManager::getInstance()->m_downloadAttachment)
    {
        emit printLog(QString::fromWCharArray(L"未开启下载附件"));
        m_success = true;
        return;
    }

    // 获取重要项目的详情
    emit printLog(QString::fromWCharArray(L"获取重要项目的详情"));
    if (!doGetDetail(client, targetZhaoBiaos))
    {
        return;
    }

    // 下载重要项目的附件
    emit printLog(QString::fromWCharArray(L"下载重要项目的附件"));
    doDownload(client, targetZhaoBiaos);

    m_success = true;
}

bool BaoPoFuWuCollectThread::doSave(const QVector<ZhaoBiao>& zhaoBiaos)
{
    // 分成3个表格保存：监理、评估、其他
    static QString jianLi = QString::fromWCharArray(L"监理");
    static QString pingGu = QString::fromWCharArray(L"评估");
    const int TYPE_COUNT = 3;
    static QString names[TYPE_COUNT] = {
        jianLi,
        pingGu,
        QString::fromWCharArray(L"其他")
    };

    QVector<ZhaoBiao> filterZhaoBiaos[TYPE_COUNT];
    for (const auto& zhaoBiao : zhaoBiaos)
    {
        if (zhaoBiao.m_title.contains(jianLi))
        {
            filterZhaoBiaos[0].append(zhaoBiao);
        }
        else if (zhaoBiao.m_title.contains(pingGu))
        {
            filterZhaoBiaos[1].append(zhaoBiao);
        }
        else
        {
            filterZhaoBiaos[2].append(zhaoBiao);
        }
    }

    for (int i=0; i<TYPE_COUNT; i++)
    {
        // 拷贝表格模板到保存目录
        QString excelFileName = QString::fromWCharArray(L"输出表格模板.xlsx");
        QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + excelFileName;
        QString destExcelFilePath = m_savedPath+"\\"+names[i]+".xlsx";
        if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
        {
            emit printLog(QString::fromWCharArray(L"拷贝表格模板到保存目录失败"));
            return false;
        }

        if (!CollectThread::doSave(destExcelFilePath, filterZhaoBiaos[i]))
        {
            return false;
        }
    }

    return true;
}

void KuangShanCollectThread::runInternal()
{
    ZhaoBiaoHttpClient client;
    client.m_cookies = StatusManager::getInstance()->getCookies();

    QDate searchBeginDate = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchBeginDate).date();
    QString searchBeginDateStr = searchBeginDate.toString("yyyy-MM-dd");
    QDate searchEndDate = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchEndDate).date();
    QString searchEndDateStr = searchEndDate.toString("yyyy-MM-dd");
    qInfo("search date from %s to %s", searchBeginDateStr.toStdString().c_str(), searchEndDateStr.toStdString().c_str());

    // 按内容附件关键词搜索
    emit printLog(QString::fromWCharArray(L"按内容附件关键词搜索"));

    SearchCondition condition;
    condition.m_beginDate = searchBeginDateStr;
    condition.m_endDate = searchEndDateStr;
    condition.m_onlyTitleField = false;
    condition.m_province = SettingManager::getInstance()->m_searchProvince;
    condition.m_keyWord = StatusManager::getInstance()->m_currentFilterKeyWord.m_contentKeyWord;
    condition.m_channels = "bidding%2Cfore%2Cpurchase%2Cfree%2Crecommend%2Cproposed%2Centrust%2Capproved%2Ccommerce%2Clisted%2Cproperty%2Cmineral%2Cland%2Cauction%2Cother";

    QVector<ZhaoBiao> zhaoBiaosByContent;
    if (!search(client, condition, zhaoBiaosByContent))
    {
        return;
    }

    emit printLog(QString::fromWCharArray(L"按内容附件关键词搜索到：%1条").arg(zhaoBiaosByContent.length()));

    // 筛选含有标题关键词项目
    QStringList titleKeywords = StatusManager::getInstance()->m_currentFilterKeyWord.m_titleKeyWord.split(" ");
    QVector<ZhaoBiao> targetZhaoBiaos;
    for (const auto& zhaoBiaoByContent : zhaoBiaosByContent)
    {
        for (const auto& titleKeyWord : titleKeywords)
        {
            if (zhaoBiaoByContent.m_title.contains(titleKeyWord))
            {
                targetZhaoBiaos.append(zhaoBiaoByContent);
                break;
            }
        }
    }

    emit printLog(QString::fromWCharArray(L"筛选含有标题关键词项目：%1条").arg(targetZhaoBiaos.length()));

    // 筛选掉标题不含关键词项目
    QString excludeKeyWord = StatusManager::getInstance()->m_currentFilterKeyWord.m_titleExcludeKeyWord;
    if (!excludeKeyWord.isEmpty())
    {
        QStringList excludeTitleKeywords = excludeKeyWord.split(" ");
        QVector<ZhaoBiao> targetZhaoBiaos2;
        for (const auto& zhaoBiao : targetZhaoBiaos)
        {
            bool found = false;
            for (const auto& keyWord : excludeTitleKeywords)
            {
                if (zhaoBiao.m_title.contains(keyWord))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                targetZhaoBiaos2.append(zhaoBiao);
            }
        }

        targetZhaoBiaos = targetZhaoBiaos2;
        emit printLog(QString::fromWCharArray(L"筛选掉标题不含关键词项目后剩余：%1条").arg(targetZhaoBiaos.length()));
    }

    if (targetZhaoBiaos.length() == 0)
    {
        m_success = true;
        return;
    }

    // 按资质关键词搜索
    emit printLog(QString::fromWCharArray(L"按资质关键词搜索"));

    QVector<ZhaoBiao> zhaoBiaosByZhiZhi;
    QString ziZhiKeyWord = StatusManager::getInstance()->m_currentFilterKeyWord.m_ziZhiKeyWord;
    if (ziZhiKeyWord.isEmpty())
    {
        emit printLog(QString::fromWCharArray(L"没有配置资质，无需搜索"));
    }
    else
    {
        condition.m_keyWord = ziZhiKeyWord;
        if (!search(client, condition, zhaoBiaosByZhiZhi))
        {
            return;
        }

        emit printLog(QString::fromWCharArray(L"按资质关键词搜索：%1条").arg(zhaoBiaosByZhiZhi.length()));
    }

    // 所属地区在设置的重要省份列表内判定为重要
    if (!SettingManager::getInstance()->m_priorityRegions.isEmpty())
    {
        QStringList provinces = SettingManager::getInstance()->m_priorityRegions.split(" ");

        int count = 0;
        for (auto& zhaoBiao : targetZhaoBiaos)
        {
            for (const auto& province : provinces)
            {
                if (zhaoBiao.m_province == province)
                {
                    zhaoBiao.m_priorityLevel = ZhaoBiao::PRIORITY_LEVEL_HIGH;
                    count++;
                    break;
                }
            }
        }
        emit printLog(QString::fromWCharArray(L"所属地区在设置的重要省份列表内判定为重要：%1条").arg(count));
    }

    // 含有资质关键词判定为重要
    if (!zhaoBiaosByZhiZhi.isEmpty())
    {
        int count = 0;
        for (auto& targetZhaoBiao : targetZhaoBiaos)
        {
            for (const auto& ziZhiZhaoBiao : zhaoBiaosByZhiZhi)
            {
                if (targetZhaoBiao.m_id == ziZhiZhaoBiao.m_id && targetZhaoBiao.m_priorityLevel != ZhaoBiao::PRIORITY_LEVEL_HIGH)
                {
                    targetZhaoBiao.m_priorityLevel = ZhaoBiao::PRIORITY_LEVEL_HIGH;
                    count++;
                    break;
                }
            }
        }
        emit printLog(QString::fromWCharArray(L"含有资质关键词判定为重要：%1条").arg(count));
    }

    // 保存采集结果
    emit printLog(QString::fromWCharArray(L"保存采集结果"));
    if (!doSave(targetZhaoBiaos))
    {
        return;
    }

    if (!StatusManager::getInstance()->m_downloadAttachment)
    {
        emit printLog(QString::fromWCharArray(L"未开启下载附件"));
        m_success = true;
        return;
    }

    // 获取重要项目的详情
    emit printLog(QString::fromWCharArray(L"获取重要项目的详情"));
    if (!doGetDetail(client, targetZhaoBiaos))
    {
        return;
    }

    // 下载重要项目的附件
    emit printLog(QString::fromWCharArray(L"下载重要项目的附件"));
    doDownload(client, targetZhaoBiaos);

    m_success = true;
}

bool KuangShanCollectThread::doSave(const QVector<ZhaoBiao>& zhaoBiaos)
{
    // 分成2个表格保存：重要、一般
    const int TYPE_COUNT = 2;
    static QString names[TYPE_COUNT] = {
        QString::fromWCharArray(L"重要"),
        QString::fromWCharArray(L"一般")
    };

    QVector<ZhaoBiao> filterZhaoBiaos[TYPE_COUNT];
    for (const auto& zhaoBiao : zhaoBiaos)
    {
        if (zhaoBiao.m_priorityLevel == ZhaoBiao::PRIORITY_LEVEL_HIGH)
        {
            filterZhaoBiaos[0].append(zhaoBiao);
        }
        else
        {
            filterZhaoBiaos[1].append(zhaoBiao);
        }
    }

    for (int i=0; i<TYPE_COUNT; i++)
    {
        // 拷贝表格模板到保存目录
        QString excelFileName = QString::fromWCharArray(L"输出表格模板.xlsx");
        QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + excelFileName;
        QString destExcelFilePath = m_savedPath+"\\"+names[i]+".xlsx";
        if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
        {
            emit printLog(QString::fromWCharArray(L"拷贝表格模板到保存目录失败"));
            return false;
        }

        if (!CollectThread::doSave(destExcelFilePath, filterZhaoBiaos[i]))
        {
            return false;
        }
    }

    return true;
}

CollectController::CollectController(QObject *parent)
    : QObject{parent}
{

}

void CollectController::run()
{
    if (m_running)
    {
        qInfo("it is collecting");
        return;
    }

    // 获取保存根目录
    m_savedRootPath = QString::fromStdWString(CImPath::GetDataPath() + L"collect\\");
    QString beginDateString = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchBeginDate).toString(QString::fromWCharArray(L"yyyy年MM月dd日"));
    QString endDateString = QDateTime::fromSecsSinceEpoch(SettingManager::getInstance()->m_searchEndDate).toString(QString::fromWCharArray(L"yyyy年MM月dd日"));
    QString folderName = beginDateString+"-"+endDateString;
    if (!QDir(m_savedRootPath+folderName).exists())
    {
        m_savedRootPath += folderName;
    }
    else
    {
        for (int i=1; i<1000000; i++)
        {
            QString newFoldName = folderName+QString("(%1)").arg(i);
            if (!QDir(m_savedRootPath+newFoldName).exists())
            {
                m_savedRootPath += newFoldName;
                break;
            }
        }
    }
    if (!QDir().mkpath(m_savedRootPath))
    {
        emit printLog(QString::fromWCharArray(L"创建保存目录失败：%1").arg(m_savedRootPath));
        emit runFinish(false, "");
        return;
    }

    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished, this, &CollectController::onRunJsCodeFinished);

    if (SettingManager::getInstance()->m_searchProvince.isEmpty())
    {
        emit printLog(QString::fromWCharArray(L"搜索省份：全国"));
    }
    else
    {
        emit printLog(QString::fromWCharArray(L"搜索省份：%1").arg(SettingManager::getInstance()->m_searchProvince));
    }

    doCollectNextKeyWords();
    m_running = true;
}

void CollectController::stop()
{
    m_requestStop = true;
    if (m_collectThread)
    {
        m_collectThread->m_exit.store(true);
    }
    else
    {
        doFinish(false);
    }
}

void CollectController::onThreadFinish()
{
    m_collectThread->deleteLater();
    m_collectThread = nullptr;

    m_nextKeyWordIndex++;
    doCollectNextKeyWords();
}

void CollectController::doCollectNextKeyWords()
{
    if (m_requestStop)
    {
        emit printLog(QString::fromWCharArray(L"停止采集"));
        doFinish(false);
        return;
    }

    if (m_nextKeyWordIndex >= StatusManager::getInstance()->m_searchFilterKeyWords.length())
    {
        emit printLog(QString::fromWCharArray(L"全部采集完成"));
        doFinish(true);
        return;
    }

    StatusManager::getInstance()->m_currentFilterKeyWord = StatusManager::getInstance()->m_searchFilterKeyWords[m_nextKeyWordIndex];
    QString currentType = StatusManager::getInstance()->m_currentFilterKeyWord.m_type;
    if (currentType == QString::fromWCharArray(L"爆破服务"))
    {
        m_collectThread = new BaoPoFuWuCollectThread();
    }
    else
    {
        m_collectThread = new KuangShanCollectThread();
    }

    m_collectThread->m_savedPath = m_savedRootPath + "\\" + currentType;
    if (!QDir().mkpath(m_collectThread->m_savedPath))
    {
        delete m_collectThread;
        m_collectThread = nullptr;

        emit printLog(QString::fromWCharArray(L"创建保存目录失败：%1").arg(m_collectThread->m_savedPath));
        doFinish(false);
        return;
    }

    connect(m_collectThread, &CollectThread::printLog, this, &CollectController::printLog);
    connect(m_collectThread, &CollectThread::updateCookie, this, &CollectController::onUpdateCookie);
    connect(m_collectThread, &CollectThread::finished, this, &CollectController::onThreadFinish);

    emit printLog(QString::fromWCharArray(L"开始采集%1类别的项目").arg(currentType));
    m_collectThread->start();
}

void CollectController::doFinish(bool success)
{
    if (m_updateCookieTimer)
    {
        m_updateCookieTimer->stop();
        m_updateCookieTimer->deleteLater();
        m_updateCookieTimer = nullptr;
    }

    disconnect(BrowserWindow::getInstance(), nullptr, this, nullptr);

    emit runFinish(success, success?m_savedRootPath:"");
}

void CollectController::onUpdateCookie(QString link, QString title)
{
    // 用浏览器访问下这个链接后，再获取cookies
    BrowserWindow::getInstance()->load(link);

    // 每隔一秒获取一次
    if (m_updateCookieTimer)
    {
        m_updateCookieTimer->stop();
        m_updateCookieTimer->deleteLater();
    }

    m_updateCookieTimer = new QTimer(this);
    connect(m_updateCookieTimer, &QTimer::timeout, [title](){
        QString jsCode = JsCodeManager::getInstance()->m_getDetailCookie;
        QString title2 = title;
        title2.replace("\"", "");
        jsCode.replace("$TITLE", title2);
        BrowserWindow::getInstance()->runJsCode("UpdateCookie", jsCode);
    });
    m_updateCookieTimer->start(1000);
}

void CollectController::onRunJsCodeFinished(const QString& id, const QVariant& result)
{
    if (id != "UpdateCookie")
    {
        return;
    }

    QString cookie = result.toString();
    if (!cookie.isEmpty())
    {
        QString jsluid = BrowserWindow::getInstance()->getHttpOnlyCookie("zb.zhaobiao.cn", "__jsluid_s");
        if (!jsluid.isEmpty())
        {
            cookie += "; __jsluid_s=" + jsluid;
        }
        qInfo("cookie with jsluid: %s", cookie.toStdString().c_str());
        StatusManager::getInstance()->setCookies(cookie);

        if (m_updateCookieTimer)
        {
            m_updateCookieTimer->stop();
            m_updateCookieTimer->deleteLater();
            m_updateCookieTimer = nullptr;
        }
    }
}

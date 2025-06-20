#include "settingmanager.h"
#include <QFile>
#include "Utility/ImPath.h"
#include "Utility/ImCharset.h"
#include "Utility/LogMacro.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SettingManager::SettingManager()
{
    load();
    loadConfig2();
    m_defaultFilterKeyWordList = loadKeyWord(QString::fromWCharArray(L"搜索关键词.txt"));
    loadFilterSetting();
}

SettingManager* SettingManager::getInstance()
{
    static SettingManager* pInstance = new SettingManager();
	return pInstance;
}

void SettingManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs.json";    
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the basic configure file : %s", strConfFilePath.c_str());
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();

    m_logLevel = root["log_level"].toInt();
    m_request_interval_ms = root["request_interval_ms"].toInt();
    m_enableWebviewLog = root["enable_webview_log"].toInt();
    m_cacheJsCode = root["cache_jscode"].toInt();
    m_browserWidth = root["browser_width"].toInt();
    m_browserHeight = root["browser_height"].toInt();
    m_debug = root["debug"].toBool();
}

void SettingManager::loadConfig2()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs2.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the basic configure2 file : %s", strConfFilePath.c_str());
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();

    m_userName = root["userName"].toString();
    m_password = root["password"].toString();
    m_searchBeginDate = root["searchBeginDate"].toInt();
    m_searchEndDate = root["searchEndDate"].toInt();
    m_priorityRegions = root["priorityRegions"].toString();
}

void SettingManager::save()
{
    QJsonObject root;
    root["userName"] = m_userName;
    root["password"] = m_password;
    root["searchBeginDate"] = m_searchBeginDate;
    root["searchEndDate"] = m_searchEndDate;
    root["priorityRegions"] = m_priorityRegions;

    QJsonDocument jsonDocument(root);
    QByteArray jsonData = jsonDocument.toJson(QJsonDocument::Indented);
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs2.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical("failed to save setting");
        return;
    }
    file.write(jsonData);
    file.close();
}

QVector<FilterKeyWord> SettingManager::loadKeyWord(QString keyWordFileName)
{
    QVector<FilterKeyWord> keyWordList;
    QString strConfFilePath = QString::fromStdWString(CImPath::GetConfPath()) + keyWordFileName;
    QFile file(strConfFilePath);
    if (!file.exists())
    {
        return keyWordList;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the keyword configure file : %s", strConfFilePath.toStdString().c_str());
        return keyWordList;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        // 跳过空行和注释行
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // 按分隔符'/'分割行
        QStringList parts = line.split('/', Qt::SkipEmptyParts);

        // 确保有3部分：类型、标题/标题不含/内容附件/资质、关键词
        if (parts.size() != 3)
            continue;

        QString type = parts[0].trimmed();
        QString category = parts[1].trimmed();
        QString keyword = parts[2].trimmed();

        // 根据类别创建或更新FilterKeyWord对象
        FilterKeyWord* existing = nullptr;
        for (FilterKeyWord& kw : keyWordList)
        {
            if (kw.m_type == type)
            {
                existing = &kw;
                break;
            }
        }

        if (!existing)
        {
            FilterKeyWord newKw;
            newKw.m_type = type;
            if (category == QString::fromWCharArray(L"标题"))
                newKw.m_titleKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"标题不含"))
                newKw.m_titleExcludeKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"内容附件"))
                newKw.m_contentKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"资质"))
                newKw.m_ziZhiKeyWord = keyword;
            keyWordList.append(newKw);
        }
        else
        {
            if (category == QString::fromWCharArray(L"标题"))
                existing->m_titleKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"标题不含"))
                existing->m_titleExcludeKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"内容附件"))
                existing->m_contentKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"资质"))
                existing->m_ziZhiKeyWord = keyword;
        }
    }

    file.close();

    return keyWordList;
}

void SettingManager::loadFilterSetting()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"人员.txt";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the filter configure file : %s", strConfFilePath.c_str());
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        // 跳过空行和注释行
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // 按分隔符'/'分割行
        QStringList parts = line.split('/');

        // 确保有4部分：发送人、文件命名、搜索省份列表、搜索关键词配置文件名称
        if (parts.size() != 4)
            continue;

        FilterSetting filterSetting;
        filterSetting.m_name = parts[0].trimmed();
        filterSetting.m_folderName = parts[1].trimmed();
        filterSetting.m_searchProvince = parts[2].trimmed();
        QString keyWordFileName = parts[3].trimmed();
        if (!keyWordFileName.isEmpty())
        {
            filterSetting.m_filterKeyWord = loadKeyWord(keyWordFileName);
        }

        if (filterSetting.m_filterKeyWord.isEmpty())
        {
            filterSetting.m_filterKeyWord = m_defaultFilterKeyWordList;
        }

        if (filterSetting.valid())
        {
            m_filterSettingList.append(filterSetting);
        }
    }

    file.close();
}

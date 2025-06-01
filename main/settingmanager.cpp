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
    loadKeyWord();
    loadExcludeKeyWord();
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

void SettingManager::loadKeyWord()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"搜索关键词.txt";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the keyword configure file : %s", strConfFilePath.c_str());
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
        QStringList parts = line.split('/', Qt::SkipEmptyParts);

        // 确保有3部分：类型、标题或内容附件、关键词
        if (parts.size() != 3)
            continue;

        QString type = parts[0].trimmed();
        QString category = parts[1].trimmed();
        QString keyword = parts[2].trimmed();

        // 根据类别创建或更新FilterKeyWord对象
        FilterKeyWord* existing = nullptr;
        for (FilterKeyWord& kw : m_filterKeyWords)
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
            else if (category == QString::fromWCharArray(L"内容附件"))
                newKw.m_contentKeyWord = keyword;
            m_filterKeyWords.append(newKw);
        }
        else
        {
            if (category == QString::fromWCharArray(L"标题"))
                existing->m_titleKeyWord = keyword;
            else if (category == QString::fromWCharArray(L"内容附件"))
                existing->m_contentKeyWord = keyword;
        }
    }

    file.close();
}

void SettingManager::loadExcludeKeyWord()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"其它关键词.txt";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the keyword configure file : %s", strConfFilePath.c_str());
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

        // 按分隔符'='分割行
        QStringList parts = line.split('=', Qt::SkipEmptyParts);

        // 确保有2部分
        if (parts.size() != 2)
            continue;

        QString type = parts[0].trimmed();
        QString keyword = parts[1].trimmed();
        if (type == QString::fromWCharArray(L"排除关键词"))
        {
            m_excludeKeyWord = keyword;
        }
    }

    file.close();
}

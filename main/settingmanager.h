#pragma once

#include <QString>
#include <QVector>

// 筛选关键词
class FilterKeyWord
{
public:
    // 名字
    QString m_name;

    // 标题关键词
    QString m_titleKeyWord;

    // 内容/附件关键词
    QString m_contentKeyWord;
};

class SettingManager
{
protected:
    SettingManager();

public:
    static SettingManager* getInstance();

private:
    void load();

public:
    // 日志级别，默认info
    int m_logLevel = 2;

    // 请求间隔，单位号码
    int m_request_interval_ms = 100;

    // 是否启用webview日志输出
    bool m_enableWebviewLog = false;

    // 标志是否要缓存JS代码，false会实时从本地文件加载
    bool m_cacheJsCode = true;

    // 浏览器宽高
    int m_browserWidth = 1920;
    int m_browserHeight = 1080;

    // 是否调试
    bool m_debug = false;

    // 筛选关键词
    QVector<FilterKeyWord> m_filterKeyWords;

    // 排除关键词，用于判断不是矿山施工类项目
    QString m_excludeKeyWord;
};

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// 筛选关键词
class FilterKeyWord
{
public:
    // 类型
    QString m_type;

    // 标题关键词
    QString m_titleKeyWord;

    // 内容/附件关键词
    QString m_contentKeyWord;

    // 资质关键词
    QString m_ziZhiKeyWord;
};

class SettingManager
{
protected:
    SettingManager();

public:
    static SettingManager* getInstance();

public:
    void save();

private:
    void load();
    void loadConfig2();
    void loadKeyWord();
    void loadOtherKeyWord();

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

    // 调试开关，开发人员测试使用
    bool m_debug = false;

    // 筛选关键词
    QVector<FilterKeyWord> m_filterKeyWords;

    // 排除关键词，用于判断不是矿山施工类项目
    QStringList m_excludeKeyWords;

    // 重要地区，用于判断重要级
    QStringList m_regions;

    // 用户名密码
    QString m_userName;
    QString m_password;

    // 搜索日期范围，时间戳
    qint64 m_searchBeginDate = 0;
    qint64 m_searchEndDate = 0;
};

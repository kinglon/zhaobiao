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

    void save();    

private:
    void load();

public:
    int m_logLevel = 2;  // info

    // 筛选关键词
    QVector<FilterKeyWord> m_filterKeyWords;

    // 排除关键词，用于判断不是矿山施工类项目
    QString m_excludeKeyWord;
};

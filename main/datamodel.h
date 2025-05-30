#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QString>
#include <QVector>

// 附件
class Attachment
{
public:
    // 文件名称
    QString m_fileName;

    // 下载链接
    QString m_downloadLink;
};

// 标
class ZhaoBiao
{
    enum FilterResult
    {
        FILTER_RESULT_NOT_NEED = 0,
        FILTER_RESULT_NEED = 1,
    };

    // 重要等级，优先级
    enum PriorityLevel
    {
        PRIORITY_LEVEL_HIGH,
        PRIORITY_LEVEL_MEDIUM,
        PRIORITY_LEVEL_LOW,
    };

public:
    // ID
    QString m_id;

    // 标题
    QString m_title;

    // 地点-省
    QString m_province;

    // 地点-市
    QString m_city;

    // 筛选结果
    int m_filterResult = FILTER_RESULT_NEED;

    // 重要等级
    int m_priorityLevel = PRIORITY_LEVEL_HIGH;

    // 链接
    QString m_link;

    // 正文内容
    QString m_content;

    // 附件
    QVector<Attachment> m_attachments;

public:
    // 获取重要性的名称
    QString getPriorityLevel()
    {
        QString name;
        switch (m_priorityLevel)
        {
        case PRIORITY_LEVEL_HIGH:
            name = QString::fromWCharArray(L"重要");
            break;
        case PRIORITY_LEVEL_MEDIUM:
            name = QString::fromWCharArray(L"次重要");
            break;
        case PRIORITY_LEVEL_LOW:
            name = QString::fromWCharArray(L"一般");
            break;
        }

        return name;
    }
};

#endif // DATAMODEL_H

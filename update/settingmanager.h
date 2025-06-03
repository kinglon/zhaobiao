#pragma once

#include <QString>

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

    // 调试开关，开发人员测试使用
    bool m_debug = false;

    // git仓库地址
    QString m_gitUrl;
};

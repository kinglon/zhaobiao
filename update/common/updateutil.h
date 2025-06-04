#ifndef UPDATEUTIL_H
#define UPDATEUTIL_H

#include <QString>

class UpdateUtil
{
public:
    UpdateUtil();

    // 启动升级程序
    // upgradeProgramPath为空，将使用默认的升级程序文件路径
    static bool startUpgradeProgram(QString upgradeProgramPath, const QStringList &arguments);

    // 检查是否需要更新
    static bool needUpgrade(QString& newVersion, QString& currentVersion);
};

#endif // UPDATEUTIL_H

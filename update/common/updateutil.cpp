#include "updateutil.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include "ImPath.h"

bool UpdateUtil::startUpgradeProgram(QString upgradeProgramPath, const QStringList &arguments)
{
    if (upgradeProgramPath.isEmpty())
    {
        upgradeProgramPath = QString::fromStdWString(CImPath::GetSoftInstallPath()+L"update.exe");
    }

    qint64 pid = 0;
    bool success = QProcess::startDetached(upgradeProgramPath, arguments, QString(), &pid);
    if (!success)
    {
        qInfo("failed to start upgrade program");
        return false;
    }

    return true;
}

bool UpdateUtil::needUpgrade(QString& srcVersion, QString& destVersion)
{
    QString updateFolderPath = QString::fromStdWString(CImPath::GetDataPath()+L"update\\");
    QFile srcVersionFile(updateFolderPath + "version.txt");
    if (!srcVersionFile.exists())
    {
        qCritical("not have version.txt in the update path");
        return false;
    }

    QString destPath = QString::fromStdWString(CImPath::GetSoftInstallPath());
    QFile destVersionFile(destPath + "version.txt");
    if (!destVersionFile.exists())
    {
        return true;
    }

    if (srcVersionFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&srcVersionFile);
        srcVersion = in.readLine().trimmed();
        srcVersionFile.close();
    }

    if (destVersionFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&destVersionFile);
        destVersion = in.readLine().trimmed();
        destVersionFile.close();
    }

    qInfo("current version: %s, new version: %s", srcVersion.toStdString().c_str(), destVersion.toStdString().c_str());

    QStringList srcVersionParts = srcVersion.split('.');
    QStringList destVersionParts = destVersion.split('.');
    if (srcVersionParts.size() != 2 || destVersionParts.size() != 2)
    {
        qCritical("version format is invalid");
        return false;
    }

    bool ok1, ok2;
    int currentMajor = destVersionParts[0].toInt(&ok1);
    int currentMinor = destVersionParts[1].toInt(&ok1);
    int newMajor = srcVersionParts[0].toInt(&ok2);
    int newMinor = srcVersionParts[1].toInt(&ok2);
    if (!ok1 || !ok2) {
        qCritical("version format is invalid");
        return false;
    }

    if (newMajor > currentMajor || (newMajor == currentMajor && newMinor > currentMinor))
    {
        return true;
    }

    return false;
}

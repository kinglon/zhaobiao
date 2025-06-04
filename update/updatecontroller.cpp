#include "updatecontroller.h"
#include "settingmanager.h"
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include <QProcess>
#include <QTextStream>
#include "ImPath.h"
#include "common/updateutil.h"

#define MAX_FAILED_COUNT 20

UpdateThread::UpdateThread(QObject *parent) : QThread(parent)
{

}

void UpdateThread::run()
{
    qInfo("the updating thread begin");
    runInternal();
    qInfo("the updating thread end");
}

void UpdateThread::runInternal()
{
    m_gitExeFilePath = QString::fromStdWString(CImPath::GetSoftInstallPath()+L"git\\bin\\git.exe");
    m_updateFolderPath = QString::fromStdWString(CImPath::GetDataPath()+L"update\\");

    // 克隆仓库
    while (true)
    {
        QThread::sleep(1);

        QString dotGitPath = m_updateFolderPath+".git";
        if (QDir(dotGitPath).exists())
        {
            emit printLog(QString::fromWCharArray(L"仓库已存在，无需克隆"));
            break;
        }

        emit printLog(QString::fromWCharArray(L"开始克隆仓库"));
        if (doClone())
        {
            emit printLog(QString::fromWCharArray(L"克隆仓库成功"));
            break;
        }
        else
        {
            emit printLog(QString::fromWCharArray(L"克隆仓库失败"));
            QThread::sleep(5);
            continue;
        }
    }

    while (true)
    {
        QThread::sleep(1);

        QString currentVersion;
        QString newVersion;
        bool needUpgrade = UpdateUtil::needUpgrade(newVersion, currentVersion);
        if (needUpgrade)
        {
            // 有新版本就先升级
            emit printLog(QString::fromWCharArray(L"当前版本：%1, 新版本：%2").arg(currentVersion, newVersion));
            doUpgrade();
        }
        else
        {
            // 拉取新版本
            doPull();
        }
    }
}

bool UpdateThread::runCommandSync(const QString &program, const QStringList &arguments, QString& output)
{
    QProcess process;
    process.start(program, arguments);

    if (!process.waitForFinished(300000))
    {
        qCritical("failed to call waitForFinished");
        return false;
    }

    // 获取输出
    QByteArray standardOutput = process.readAllStandardOutput();
    QByteArray errorOutput = process.readAllStandardError();
    output = QString::fromUtf8(standardOutput) + "\r\n" + QString::fromUtf8(errorOutput);
    return true;
}

bool UpdateThread::doClone()
{
    QDir(m_updateFolderPath).removeRecursively();
    QThread::msleep(200);

    QStringList arguments;
    arguments.append("clone");
    arguments.append(SettingManager::getInstance()->m_gitUrl);
    arguments.append(m_updateFolderPath);

    QString output;
    if (runCommandSync(m_gitExeFilePath, arguments, output))
    {
        if (output.contains("Cloning into") && output.contains("Receiving objects: 100%"))
        {
            return true;
        }
        else
        {
            qCritical("failed to clone, error: %s", output.toStdString().c_str());
        }
    }

    return false;
}

void UpdateThread::doUpgrade()
{
    while (true)
    {
        QThread::sleep(3);

        // 等待主程序退出
        doWaitMainProgramExit();
        emit printLog(QString::fromWCharArray(L"主程序已退出"));

        // 拷贝升级文件
        QString destPath = QString::fromStdWString(CImPath::GetSoftInstallPath());
        if (doCopyDirectory(m_updateFolderPath, destPath))
        {
            // 最后拷贝version.txt文件
            QString srcVersionFile = m_updateFolderPath + "version.txt";
            QString destVersionFile = destPath + "version.txt";
            if (QFile::exists(destVersionFile))
            {
                QFile::remove(destVersionFile);
            }

            if (!QFile::copy(srcVersionFile, destVersionFile))
            {
                emit printLog(QString::fromWCharArray(L"拷贝文件失败：version.txt"));
                continue;
            }

            emit printLog(QString::fromWCharArray(L"拷贝升级文件完成"));
            break;
        }
    }
}

void UpdateThread::doWaitMainProgramExit()
{
    while (true)
    {
        // 检测主程序的互斥对象判断
        const wchar_t* mutexName = L"{4ED3367A-D83A-4D0A-6623-158D74420098}";
        HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, mutexName);
        if (hMutex == NULL)
        {
            return;
        }

        CloseHandle(hMutex);
        emit printLog(QString::fromWCharArray(L"等待主程序退出"));
        QThread::sleep(5);
    }
}

bool UpdateThread::doCopyDirectory(const QString &srcPath, const QString &dstPath)
{
    QDir srcDir(srcPath);
    QDir dstDir(dstPath);

    // 获取源目录中的所有文件和目录
    QFileInfoList entries = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

    foreach (const QFileInfo &entry, entries)
    {
        QString srcFilePath = entry.filePath();
        QString dstFilePath = dstPath + QDir::separator() + entry.fileName();

        if (entry.isDir())
        {
            // 跳过.git目录
            if (entry.fileName() == ".git")
            {
                continue;
            }

            // 递归拷贝子目录
            if (!doCopyDirectory(srcFilePath, dstFilePath))
            {
                return false;
            }
        }
        else
        {
            // 跳过version.txt文件
            if (entry.fileName() == "version.txt")
            {
                continue;
            }

            // 拷贝文件
            if (QFile::exists(dstFilePath))
            {
                QFile::remove(dstFilePath);
            }

            if (!QFile::copy(srcFilePath, dstFilePath))
            {
                emit printLog(QString::fromWCharArray(L"拷贝文件失败：%1").arg(entry.fileName()));
                return false;
            }
        }
    }

    return true;
}

void UpdateThread::doPull()
{
    while (true)
    {
        QThread::sleep(1);

        // 重置仓库
        QStringList arguments;
        arguments.append("-C");
        arguments.append(m_updateFolderPath);
        arguments.append("reset");
        arguments.append("--hard");

        QString output;
        runCommandSync(m_gitExeFilePath, arguments, output);

        // 清除未跟踪文件
        arguments.clear();
        arguments.append("-C");
        arguments.append(m_updateFolderPath);
        arguments.append("clean");
        arguments.append("-fd");
        runCommandSync(m_gitExeFilePath, arguments, output);

        // 拉取
        arguments.clear();
        arguments.append("-C");
        arguments.append(m_updateFolderPath);
        arguments.append("pull");
        if (!runCommandSync(m_gitExeFilePath, arguments, output))
        {
            emit printLog(QString::fromWCharArray(L"运行拉取命令失败"));
        }
        else
        {
            if (output.contains("up to date"))
            {
                emit printLog(QString::fromWCharArray(L"已经是最新版本"));
            }
            else if (output.contains("Fast-forward"))
            {
                emit printLog(QString::fromWCharArray(L"新版本已更新到本地"));
                return;
            }
            else
            {
                qCritical("failed to pull, error: %s", output.toStdString().c_str());
                emit printLog(QString::fromWCharArray(L"拉取新版本失败"));
            }
        }

        QThread::sleep(60);
    }
}

UpdateController::UpdateController(QObject *parent)
    : QObject{parent}
{

}

void UpdateController::run()
{
    if (m_updateThread)
    {
        qInfo("it is updating");
        return;
    }

    m_updateThread = new UpdateThread();
    connect(m_updateThread, &UpdateThread::printLog, this, &UpdateController::printLog);
    connect(m_updateThread, &UpdateThread::finished, this, &UpdateController::onThreadFinish);
    m_updateThread->start();
}

void UpdateController::onThreadFinish()
{    
    m_updateThread->deleteLater();
    m_updateThread = nullptr;
}

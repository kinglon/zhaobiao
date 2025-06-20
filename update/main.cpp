﻿#include "mainwindow.h"

#include <QApplication>
#include "LogUtil.h"
#include "DumpUtil.h"
#include "ImPath.h"
#include "settingmanager.h"

CLogUtil* g_dllLog = nullptr;

QtMessageHandler originalHandler = nullptr;

void logToFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_dllLog)
    {
        ELogLevel logLevel = ELogLevel::LOG_LEVEL_ERROR;
        if (type == QtMsgType::QtDebugMsg)
        {
            logLevel = ELogLevel::LOG_LEVEL_DEBUG;
        }
        else if (type == QtMsgType::QtInfoMsg || type == QtMsgType::QtWarningMsg)
        {
            logLevel = ELogLevel::LOG_LEVEL_INFO;
        }

        QString newMsg = msg;
        newMsg.remove(QChar('%'));
        g_dllLog->Log(context.file? context.file: "", context.line, logLevel, newMsg.toStdWString().c_str());
    }

    if (originalHandler)
    {
        (*originalHandler)(type, context, msg);
    }
}

int main(int argc, char *argv[])
{
    // 单实例
    const wchar_t* mutexName = L"{4ED4467A-D83A-4D0A-6623-158D74420098}";
    HANDLE mutexHandle = CreateMutexW(nullptr, TRUE, mutexName);
    if (mutexHandle == nullptr)
    {
        return 0;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // 默认无参数的时候，通知显示窗口
        if (argc == 1)
        {
            g_dllLog = CLogUtil::GetLog(L"update2");
            qInstallMessageHandler(logToFile);
            IpcWorker::sendData(IPC_KEY, "showWindow");
        }
        return 0;
    }

    g_dllLog = CLogUtil::GetLog(L"update");

    // 初始化崩溃转储机制
    CDumpUtil::SetDumpFilePath(CImPath::GetDumpPath().c_str());
    CDumpUtil::Enable(true);

    // 设置日志级别
    int nLogLevel = SettingManager::getInstance()->m_logLevel;
    g_dllLog->SetLogLevel((ELogLevel)nLogLevel);
    originalHandler = qInstallMessageHandler(logToFile);

    if (SettingManager::getInstance()->m_gitUrl.isEmpty())
    {
        qInfo("git url not configured");
        return 0;
    }

    qputenv("QT_FONT_DPI", "100");

    QApplication a(argc, argv);
    MainWindow w;
    if (argc > 1 && QString::fromUtf8(argv[1]).contains("hide"))
    {
        // 明确参数指定隐藏时，不显示窗口，后台运行
        w.hide();
    }
    else
    {
        w.show();
    }
    return a.exec();
}

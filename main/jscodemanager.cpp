#include "jscodemanager.h"
#include <QFile>
#include "Utility/ImPath.h"
#include "Utility/ImCharset.h"
#include "Utility/LogMacro.h"

JsCodeManager::JsCodeManager()
{
    load();
}

JsCodeManager* JsCodeManager::getInstance()
{
    static JsCodeManager* instance = new JsCodeManager();
    return instance;
}

void JsCodeManager::load()
{
    QString jsRootPath = QString::fromStdWString(CImPath::GetConfPath()) + "Js\\";

    QString getLoginInfoFilePath = jsRootPath + "getLoginInfo.js";
    QFile getLoginInfoFile(getLoginInfoFilePath);
    if (getLoginInfoFile.open(QIODevice::ReadOnly))
    {
        m_getLoginInfo = QString::fromUtf8(getLoginInfoFile.readAll());
        getLoginInfoFile.close();
    }

    QString getDetailCookieFilePath = jsRootPath + "getDetailCookie.js";
    QFile getDetailCookieFile(getDetailCookieFilePath);
    if (getDetailCookieFile.open(QIODevice::ReadOnly))
    {
        m_getDetailCookie = QString::fromUtf8(getDetailCookieFile.readAll());
        getDetailCookieFile.close();
    }
}

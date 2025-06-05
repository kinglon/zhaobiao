#ifndef JSCODEMANAGER_H
#define JSCODEMANAGER_H

#include <QString>

class JsCodeManager
{
protected:
    JsCodeManager();

public:
    static JsCodeManager* getInstance();

private:
    void load();

public:
    QString m_getLoginInfo;

    QString m_fillUserPassword;

    QString m_getDetailCookie;
};

#endif // JSCODEMANAGER_H

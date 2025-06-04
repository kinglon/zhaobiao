#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <QObject>
#include <QThread>
#include <QVector>

class UpdateThread: public QThread
{
    Q_OBJECT

public:
    explicit UpdateThread(QObject *parent = nullptr);

    void run() override;

private:
    void runInternal();

    bool runCommandSync(const QString &program, const QStringList &arguments, QString& output);

    bool doClone();

    void doUpgrade();

    // 等待主程序退出
    void doWaitMainProgramExit();

    // 拷贝目录，排除.git目录和version.txt文件
    bool doCopyDirectory(const QString &srcPath, const QString &dstPath);

    void doPull();

    bool needUpgrade();    

signals:
    void printLog(QString content);

private:
    QString m_gitExeFilePath;

    // 升级文件夹目录，尾部有后划线
    QString m_updateFolderPath;
};

class UpdateController : public QObject
{
    Q_OBJECT
public:
    explicit UpdateController(QObject *parent = nullptr);

    void run();

signals:
    void printLog(QString content);

private slots:
    void onThreadFinish();

private:
    UpdateThread* m_updateThread = nullptr;
};

#endif // UPDATECONTROLLER_H

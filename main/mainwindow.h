#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include "logincontroller.h"
#include "collectcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCtrlDShortcut();

    void onPrintLog(QString content);

    void onLoginButtonClicked();

    void onStartButtonClicked();

    void onStopButtonClicked();

    void onOpenFolderButtonClicked();

private:
    void initCtrls();

    void initBrowser();

    void updateButtonStatus();

    void initController();

    void saveSetting();

    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    LoginController m_loginController;

    CollectController* m_collectController = nullptr;

    // 标志是否正在采集
    bool m_collecting = false;

    // 最后一次采集保存目录
    QString m_lastSavedPath;
};
#endif // MAINWINDOW_H

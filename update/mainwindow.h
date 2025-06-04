#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include "updatecontroller.h"
#include "ipcworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define IPC_KEY  "{4ED33E4A-ee3A-920A-8523-888D74420098}"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initController();

private slots:
    void onPrintLog(QString content);

    void onIpcDataArrive(QString data);

    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    IpcWorker m_ipcWorker;

    UpdateController m_updateController;
};
#endif // MAINWINDOW_H

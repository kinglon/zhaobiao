#ifndef COLLECTCONTROLLER_H
#define COLLECTCONTROLLER_H

#include <QObject>

class CollectController : public QObject
{
    Q_OBJECT
public:
    explicit CollectController(QObject *parent = nullptr);

    void run();

    void stop();

signals:
    void printLog(QString content);

    void runFinish(bool success, QString savedPath);
};

#endif // COLLECTCONTROLLER_H

#ifndef BRIGHTTHREAD_H
#define BRIGHTTHREAD_H

#include <QThread>
#include <QMutex>
class QGSettings;

class BrightThread : public QThread
{
    Q_OBJECT
public:
    BrightThread(QObject *parent = nullptr, double bright = 100.0);
//    BrightThread(QObject *parent = nullptr);
    ~BrightThread();
    void stopImmediately();

protected:
    void run();

private:
    double brightness;
    double currentBrightness;
    QGSettings *mpowerSettings;
    bool m_isCanRun;
    QMutex m_lock;
};

#endif // BRIGHTTHREAD_H

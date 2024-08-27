#ifndef RECOGNITIONTHREADWORKER_H
#define RECOGNITIONTHREADWORKER_H

#include <QThread>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QDateTime>
#include <QRandomGenerator>
#include <QProcess>
#include <QDir>

#include "YoloV2.h"

class RecognitionThreadWorker : public QThread
{
    Q_OBJECT
public:
    explicit RecognitionThreadWorker(QString RunningFolder, QObject *parent = nullptr);

protected:
    void run() override;

signals:
    void m_errorMessage(QString Msg);
    void m_recognitionResult(QStringList RecognitionResults);

public slots:
    void m_handleDetectionResult(QStringList ResultArray);

private:
    bool m_loadModel();

    QString m_runningFolder;

    QMutex m_mutex;
    QQueue<QStringList> m_detectionResults;

    YOLOv2Detector *m_detector;
};

#endif // RECOGNITIONTHREADWORKER_H

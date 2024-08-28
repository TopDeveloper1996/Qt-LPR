#ifndef DETECTIONTHREADWORKER_H
#define DETECTIONTHREADWORKER_H

#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

#include "YoloV2.h"

struct DetectionKeyPoint
{
    int top{0};
    int bottom{0};

    DetectionKeyPoint(int _top, int _bottom)
        : top(_top), bottom(_bottom)
    {}
};

class DetectionThreadWorker : public QThread
{
    Q_OBJECT
public:
    explicit DetectionThreadWorker(QString RunningFolder, QString Direction, QString CameraUrl, QObject *parent = nullptr);
protected:
    void run() override;
signals:
    void m_currentFrame(QPixmap Frame);
    void m_detectionResult(QStringList DetectionResults);
    void m_errorMessage(QString Msg);
private slots:
    void m_handleTimeout();
private:
    bool m_loadModel();

    QVector<DetectionKeyPoint> m_keyPoints;

    QTimer *m_timer;

    QString m_runningFolder, m_direction, m_cameraUrl;
    QStringList m_detectionResultFiles;

    YOLOv2Detector *m_detector;
};

#endif // DETECTIONTHREADWORKER_H

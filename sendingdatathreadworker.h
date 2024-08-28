#ifndef SENDINGDATATHREADWORKER_H
#define SENDINGDATATHREADWORKER_H

#include <QThread>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QEventLoop>
#include <QMessageBox>

class SendingDataThreadWorker : public QThread
{
    Q_OBJECT
public:
    explicit SendingDataThreadWorker(QString RunningFolder, QString ServerUrl, QString Token, QObject *parent = nullptr);
protected:
    void run() override;
signals:
    void m_errorMessage(QString Msg);

public slots:
    void m_handleRecognitionResult(QStringList RecognitionResult);
    void m_handleResponse(QNetworkReply *reply);
    // void m_handleError(QNetworkReply *reply, QNetworkReply::NetworkError error);

    void processNextResult();

private:
    QHttpPart createTextPart(const QString& name, const QString& value);
    QHttpPart createImagePart(const QString& name, const QString& imageFile);

    QString m_serverUrl, m_token, m_runningFolder;
    QNetworkAccessManager *m_networkManager;

    QMutex m_mutex;
    QQueue<QStringList> m_recognitionResults;
};

#endif // SENDINGDATATHREADWORKER_H

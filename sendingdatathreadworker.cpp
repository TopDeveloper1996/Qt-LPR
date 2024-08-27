#include "sendingdatathreadworker.h"

SendingDataThreadWorker::SendingDataThreadWorker(QString ServerUrl, QString Token, QObject *parent)
    : QThread{parent}, m_serverUrl(ServerUrl), m_token(Token)
{}

void SendingDataThreadWorker::run()
{
    // Create the QNetworkAccessManager in the thread's context
    m_networkManager = new QNetworkAccessManager();
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &SendingDataThreadWorker::m_handleResponse);

    exec(); // Start the event loop for this thread
}

void SendingDataThreadWorker::m_handleRecognitionResult(QStringList RecognitionResult)
{
    QMutexLocker locker(&m_mutex);
    m_recognitionResults.enqueue(RecognitionResult);

    // Trigger sending the data in the worker thread
    QMetaObject::invokeMethod(this, "processNextResult", Qt::QueuedConnection);
}

void SendingDataThreadWorker::processNextResult()
{
    QMutexLocker locker(&m_mutex);

    if (m_recognitionResults.isEmpty()) {
        return;  // No data to send
    }

    QStringList recognitionResult = m_recognitionResults.dequeue();
    locker.unlock();  // Unlock the mutex while processing

    QString vehicleImage = recognitionResult.at(0);
    QString plateImage = recognitionResult.at(1);
    QString number = recognitionResult.at(2);

    QStringList data = vehicleImage.split("#");

    QString camera = data.at(1);
    QString time = QDateTime::fromString(data.at(2), "yyyy-MM-dd-hh-mm-ss-zzz").toString(Qt::ISODateWithMs);
    QString direction = data.at(3);

    // Prepare the multipart/form-data request
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    multiPart->append(createTextPart("camera", camera));
    multiPart->append(createTextPart("time", time));
    multiPart->append(createTextPart("direction", direction));
    multiPart->append(createTextPart("plateNumber", number));

    multiPart->append(createImagePart("vehicle", vehicleImage));
    multiPart->append(createImagePart("plate", plateImage));

    // Create the network request
    QNetworkRequest request((QUrl(m_serverUrl)));
    request.setRawHeader("token", m_token.toUtf8());

    // Send the request asynchronously
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);  // Ensures the multipart data is deleted with the reply

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // Once finished, schedule the next result for processing
        reply->deleteLater();
        QMetaObject::invokeMethod(this, "processNextResult", Qt::QueuedConnection);
    });
}

void SendingDataThreadWorker::m_handleResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "Response received: " << response;
        // Process the response if needed
    } else {
        qDebug() << "Error in reply: " << reply->errorString();
    }
}

QHttpPart SendingDataThreadWorker::createTextPart(const QString &name, const QString &value)
{
    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + name + "\""));
    textPart.setBody(value.toUtf8());
    return textPart;
}

QHttpPart SendingDataThreadWorker::createImagePart(const QString &name, const QString &imageFile)
{
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + name + "\"; filename=\"" + QFileInfo(imageFile).fileName() + "\""));

    QFile *file = new QFile(imageFile);
    if (file->open(QIODevice::ReadOnly)) {
        imagePart.setBodyDevice(file);
        connect(file, &QFile::aboutToClose, file, &QFile::deleteLater);
    } else {
        delete file;
        qDebug() << "Failed to open image file:" << imageFile;
    }

    return imagePart;
}

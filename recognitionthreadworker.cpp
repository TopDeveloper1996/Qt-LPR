#include "recognitionthreadworker.h"

string get_string_from_recognition(vector<Detection> Recognitions)
{
    if (Recognitions.empty()) {
        return ":";
    }
    vector<Detection> tmp = Recognitions;
    std::sort(tmp.begin(), tmp.end(),
              [](const Detection &a, const Detection &b) {
                  return a.box.x < b.box.x;
              });

    vector<Detection> resultNumbers;
    for (unsigned long i=0;i<tmp.size();i++) {
        if(i==0){
            resultNumbers.push_back(tmp[i]);
            continue;
        }
        if((tmp[i].box.x - tmp[i-1].box.x) < tmp[i].box.height/20.0){
            if(tmp[i].confidence > tmp[i-1].confidence){
                resultNumbers.pop_back();
                resultNumbers.push_back(tmp[i]);
            }
        }else
            resultNumbers.push_back(tmp[i]);
    }
    string result="";
    for (unsigned long i=0;i<resultNumbers.size();i++) {
        result += resultNumbers.at(i).className;
    }
    if(5<result.length() && result.length()<9)
        return result;
    else
        return ":";
}

RecognitionThreadWorker::RecognitionThreadWorker(QString RunningFolder, QObject *parent)
    : QThread{parent}, m_runningFolder(RunningFolder)
{

}

void RecognitionThreadWorker::run()
{
    try {

        if(!m_loadModel()){
            throw std::runtime_error("Loading model failed...");
        }

        while (!isInterruptionRequested()) {

            QMutexLocker locker(&m_mutex);

            while (!m_detectionResults.isEmpty()) {

                QStringList detectionResults = m_detectionResults.dequeue();
                QStringList recognizedNumbers, toRemove;

                for(QString fileName:detectionResults){

                    cv::Mat img = cv::imread(fileName.toStdString());

                    if(!img.empty()){

                        // Extraction data from vehicle image file name
                        QStringList detectionData = fileName.split("#");

                        QString plate_x = detectionData.at(4);
                        QString plate_y = detectionData.at(5);
                        QString plate_w = detectionData.at(6);
                        QString plate_h = detectionData.at(7);

                        // Crop plate section from vehicle image file
                        cv::Mat plateSection = img(cv::Rect(plate_x.toInt(), plate_y.toInt(), plate_w.toInt(), plate_h.toInt()));
                        vector<Detection> detections = m_detector->detect(plateSection);

                        QString recognizedNumber = QString::fromStdString(get_string_from_recognition(detections));
                        if(recognizedNumber != ":"){
                            recognizedNumbers.append(recognizedNumber);

                            detectionData.replace(0, m_runningFolder + "/temp/plate");
                            QString plateFileName = detectionData.join("#");

                            if(!cv::imwrite(plateFileName.toStdString(), plateSection)){
                                throw std::runtime_error("Failed to save temp plate image file...");
                            }
                        }else{
                            QFile::remove(fileName);
                            toRemove.append(fileName);
                        }
                    }else{
                        throw std::runtime_error("Failed to read temp vehicle image file...");
                    }
                }

                for (QString fileName : toRemove) {
                    detectionResults.removeOne(fileName);
                }

                if(recognizedNumbers.length() != 0){

                    // Get final recognition result as string
                    QMap<QString, int> countMap;
                    for (const QString& item : recognizedNumbers) {
                        countMap[item]++;
                    }

                    QString finalNumber = countMap.key(*std::max_element(countMap.constBegin(), countMap.constEnd()));

                    // Remove temp files except one to send to server, one vehicle and one plate
                    int randomIndex = QRandomGenerator::global()->bounded(detectionResults.size());
                    QString selectedFile = detectionResults[randomIndex];

                    for(QString tempFile: detectionResults){
                        if(tempFile != selectedFile){
                            QFile::remove(tempFile);
                            QFile::remove(tempFile.replace("vehicle#", "plate#"));
                        }
                    }

                    // send data to sending thread
                    QString detectedVehicleImage = selectedFile;
                    QString detectedPlateImage = selectedFile.replace("vehicle#", "plate#");
                    QString recognizedNumber = finalNumber;

                    QStringList recognitionResult;
                    recognitionResult << detectedVehicleImage << detectedPlateImage << recognizedNumber;

                    emit m_recognitionResult(recognitionResult);
                }
            }

            if(isInterruptionRequested()){
                break;
            }
        }
    } catch (const std::exception& ex) {
        std::cout << ex.what() <<std::endl;
        QString errorMessage = QString::fromStdString(ex.what());
        emit m_errorMessage(errorMessage);
    }
}

void RecognitionThreadWorker::m_handleDetectionResult(QStringList ResultArray)
{
    try {
        QMutexLocker locker(&m_mutex);
        m_detectionResults.enqueue(ResultArray);
    } catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}

bool RecognitionThreadWorker::m_loadModel()
{
    try {
        m_detector = new YOLOv2Detector((m_runningFolder + "/models/recognition.cfg").toStdString(), (m_runningFolder + "/models/recognition.weights").toStdString(), (m_runningFolder + "/models/recognition.names").toStdString(), true, cv::Size(240, 120), 0.5, 0.4);
    } catch (const std::exception& ex) {
        std::cout << ex.what() <<std::endl;
        return false;
    }
    return true;
}

#include "detectionthreadworker.h"

Detection get_max_confidence_detection(vector<Detection> Detections)
{
    if (Detections.empty()) {
        return Detection();
    }
    auto maxElement = std::max_element(Detections.begin(), Detections.end(),
                                       [](const Detection &a, const Detection &b) {
                                           return a.confidence < b.confidence;
                                       });
    return *maxElement;
}

DetectionThreadWorker::DetectionThreadWorker(QString RunningFolder, QString Direction, QString CameraUrl, QObject *parent)
    : QThread{parent}, m_runningFolder(RunningFolder), m_direction(Direction), m_cameraUrl(CameraUrl)
{}

void DetectionThreadWorker::run()
{
    try {

        // detection model loading
        if(!m_loadModel()){
            throw std::runtime_error("Loading model failed...");
        }
        // camera status check
        cv::VideoCapture camera(m_cameraUrl.toStdString());
        cv::Mat cameraFrame;

        if(!camera.isOpened()){
            throw std::runtime_error("Opening camera failed...");
        }

        //preparation of key points for saving vehicle images
        int frameHeight = camera.get(cv::CAP_PROP_FRAME_HEIGHT);

        m_keyPoints.append(DetectionKeyPoint(int(frameHeight / 4), int(frameHeight / 20 * 7)));
        m_keyPoints.append(DetectionKeyPoint(int(frameHeight / 20 * 7), int(frameHeight / 20 * 9))); // top
        m_keyPoints.append(DetectionKeyPoint(int(frameHeight / 20 * 9), int(frameHeight / 20 * 11)));
        m_keyPoints.append(DetectionKeyPoint(int(frameHeight / 20 * 11), int(frameHeight / 20 * 13)));
        m_keyPoints.append(DetectionKeyPoint(int(frameHeight / 20 * 13), int(frameHeight / 4 * 3)));  // bottom

        int curPosition = -1;
        int timer = 0;

        // preparation for plate tracking
        // int prev_x = 0, prev_y = 0;
        // bool isSamePlate = false;

        // main process
        while (!isInterruptionRequested()) {

            // check whether camera frames are loaded correctly
            if(!camera.read(cameraFrame) || cameraFrame.empty()){
                throw std::runtime_error("Error reading frames...");
            }

            // get inference
            vector<Detection> detections = m_detector->detect(cameraFrame);

            // pick one detection result that has the highest confidence
            Detection detection = get_max_confidence_detection(detections);

            // validation of selected detection area
            if(detection.confidence != 0 && detection.box.height < detection.box.width && detection.box.y < m_keyPoints.at(4).bottom && detection.box.y > m_keyPoints.at(0).top){

                //initialize timer
                timer = 0;
                // Save vehicle images to recognize numbers from them
                for(int i =0; i< 5; i++){
                    if(m_keyPoints.at(i).top < detection.box.y && m_keyPoints.at(i).bottom > detection.box.y && curPosition != i){

                        curPosition = i;

                        QString tempFolderPath = m_runningFolder + "/temp";
                        QDir dir(tempFolderPath);
                        if(!dir.exists()){
                            if(dir.mkpath(".")){
                                std::cout << "Temp directory was created..." <<std::endl;
                            }else{
                                throw std::runtime_error("Failed to create temp directory...");
                            }
                        }
                        QString vehicleImage = tempFolderPath + "/vehicle#" + m_cameraUrl.replace(QRegularExpression("\\D+"), "") + "#" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz") + "#" + m_direction + "#" +QString::number(detection.box.x) + "#" +QString::number(detection.box.y) + "#" +QString::number(detection.box.width) + "#" +QString::number(detection.box.height) + "#.jpg";
                        if(!cv::imwrite(vehicleImage.toStdString(), cameraFrame)){
                            throw std::runtime_error("Failed to save temp vehicle image files...");
                        }

                        m_detectionResultFiles.append(vehicleImage);

                        break;
                    }
                }

                Drawer::drawDetection(detection, cameraFrame);
            }else{
                timer ++;
            }

            if(timer > 30){
                m_handleTimeout();
                timer = 0;
            }
            // emit signal to display frame in label
            double aspectRatio = static_cast<double>(cameraFrame.cols) / cameraFrame.rows;
            int desiredWidth = 400;
            int desiredHeight = static_cast<int>(desiredWidth / aspectRatio);

            cv::resize(cameraFrame, cameraFrame, cv::Size(desiredWidth, desiredHeight));

            cv::cvtColor(cameraFrame, cameraFrame, COLOR_BGR2RGB);
            QPixmap currentFrame = QPixmap::fromImage(QImage((unsigned char*)cameraFrame.data, cameraFrame.cols, cameraFrame.rows, cameraFrame.step, QImage::Format_RGB888));

            emit m_currentFrame(currentFrame);

            if(isInterruptionRequested()){
                break;
            }
        }
        camera.release();

        exec();

    } catch (const std::exception& ex) {
        std::cout << ex.what() <<std::endl;
        QString errorMessage = QString::fromStdString(ex.what());
        emit m_errorMessage(errorMessage);
    }
}

void DetectionThreadWorker::m_handleTimeout()
{
    emit m_detectionResult(m_detectionResultFiles);
    m_detectionResultFiles.clear();
}

bool DetectionThreadWorker::m_loadModel()
{
    try {
        m_detector = new YOLOv2Detector((m_runningFolder + "/models/detection.cfg").toStdString(), (m_runningFolder + "/models/detection.weights").toStdString(), (m_runningFolder + "/models/detection.names").toStdString(), true, cv::Size(224, 224), 0.8, 0.4);
    } catch (const std::exception& ex) {
        std::cout << ex.what() <<std::endl;
        return false;
    }
    return true;
}

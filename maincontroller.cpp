#include "maincontroller.h"
#include "./ui_maincontroller.h"

MainController::MainController(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainController)
{
    ui->setupUi(this);
    m_recognitionThread = new RecognitionThreadWorker("C:\\cpm");
    m_recognitionThread->start();

    cur_idx = 1;
    QFile file("C:\\cpm\\settings.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(nullptr, "Failed to load setting...", "Please configure needed information!");
        return;
    }
    QTextStream in(&file);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if(line != ""){
            QStringList settingInfo = line.split("#");
            if(settingInfo.at(0) == "token"){
                QString serverURL = settingInfo.at(1).split("&").at(0).split("=").at(1);
                QString token = settingInfo.at(1).split("&").at(1).split("=").at(1);

                // m_sendingThread = new SendingDataThreadWorker("C:\\cpm", serverURL, token);
                // connect(m_recognitionThread, &RecognitionThreadWorker::m_recognitionResult, m_sendingThread, &SendingDataThreadWorker::m_handleRecognitionResult);
                // m_sendingThread->start();

            }else{
                DetectionThreadWorker *thead = new DetectionThreadWorker("C:\\cpm", settingInfo.at(0), settingInfo.at(1));
                thead->setProperty("index", cur_idx);

                connect(thead, &DetectionThreadWorker::m_currentFrame, [this, thead](QPixmap frame){
                    QMetaObject::invokeMethod(this, [this, frame, thead](){
                        QObject* childObject = this->findChild<QObject*>("lbl_camera_" + QString::number(thead->property("index").toInt()));
                        qobject_cast<QLabel*>(childObject)->setPixmap(frame);
                    }, Qt::QueuedConnection);
                });

                connect(thead, &DetectionThreadWorker::m_errorMessage, [this, thead](QString ErrorMessage){
                    QMetaObject::invokeMethod(this, [this, ErrorMessage, thead](){
                        QObject* childObject = this->findChild<QObject*>("lbl_camera_" + QString::number(thead->property("index").toInt()));
                        qobject_cast<QLabel*>(childObject)->setText(ErrorMessage);
                        thead->quit();
                    }, Qt::QueuedConnection);
                });

                connect(thead, &DetectionThreadWorker::m_detectionResult, m_recognitionThread, &RecognitionThreadWorker::m_handleDetectionResult);
                cur_idx ++;
                thead->start();
            }

        }
        qDebug() << line;
    }

    file.close();
}

MainController::~MainController()
{
    delete ui;
}

void MainController::on_btn_add_clicked()
{
    //get new camera URL from edit and create thread that works on detection plates
    QString Direction = ui->cbx_location->currentText();
    QString CameraUrl = ui->edt_url->text();

    DetectionThreadWorker *thead = new DetectionThreadWorker("C:\\cpm", Direction, CameraUrl);
    thead->setProperty("index", cur_idx);

    connect(thead, &DetectionThreadWorker::m_currentFrame, [this, thead](QPixmap frame){
        QMetaObject::invokeMethod(this, [this, frame, thead](){
            QObject* childObject = this->findChild<QObject*>("lbl_camera_" + QString::number(thead->property("index").toInt()));
            qobject_cast<QLabel*>(childObject)->setPixmap(frame);
        }, Qt::QueuedConnection);
    });

    connect(thead, &DetectionThreadWorker::m_errorMessage, [this, thead](QString ErrorMessage){
        QMetaObject::invokeMethod(this, [this, ErrorMessage, thead](){
            QObject* childObject = this->findChild<QObject*>("lbl_camera_" + QString::number(thead->property("index").toInt()));
            qobject_cast<QLabel*>(childObject)->setText(ErrorMessage);
            thead->quit();
        }, Qt::QueuedConnection);
    });

    connect(thead, &DetectionThreadWorker::m_detectionResult, m_recognitionThread, &RecognitionThreadWorker::m_handleDetectionResult);
    cur_idx ++;
    thead->start();
}


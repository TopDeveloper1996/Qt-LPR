#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QWidget>
#include <qlabel.h>
#include <QMessageBox>

#include "detectionthreadworker.h"
#include "recognitionthreadworker.h"
#include "sendingdatathreadworker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainController;
}
QT_END_NAMESPACE

class MainController : public QWidget
{
    Q_OBJECT

public:
    MainController(QWidget *parent = nullptr);
    ~MainController();

private slots:
    void on_btn_add_clicked();

private:
    Ui::MainController *ui;

    int cur_idx;

    RecognitionThreadWorker *m_recognitionThread;
    SendingDataThreadWorker *m_sendingThread;
};
#endif // MAINCONTROLLER_H

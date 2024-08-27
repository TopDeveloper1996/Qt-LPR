#include "YoloV2.h"

YOLOv2Detector::YOLOv2Detector(const string& cfgFile, const string& weightsFile, const string& classNamesFile, const bool applyNMS, Size size, float conf_thresh, float nms_thresh) {
    net = readNetFromDarknet(cfgFile, weightsFile);
    net.setPreferableBackend(DNN_BACKEND_OPENCV);
    net.setPreferableTarget(DNN_TARGET_CPU);
    outputLayerNames = net.getUnconnectedOutLayersNames();
    loadClassNames(classNamesFile);
    m_applyNMS = applyNMS;
    m_size = size;
    confidenceThreshold = conf_thresh;
    nmsThreshold = nms_thresh;
}

vector<Detection> YOLOv2Detector::detect(const Mat& frame) {
    Mat blob;
    blobFromImage(frame, blob, 1 / 255.0, m_size, Scalar(0, 0, 0), true, false);
    net.setInput(blob);
    vector<Mat> outs;
    net.forward(outs, outputLayerNames);
    
    vector<Detection> detections;
    postprocess(frame, outs, detections);
    return detections;
}

void YOLOv2Detector::postprocess(const Mat& frame, const vector<Mat>& outs, vector<Detection>& detections) {
    Size frameSize = frame.size();

    vector<float> confidences;
    vector<Rect> boxes;

    for (size_t i = 0; i < outs.size(); ++i) {
        float* data = (float*)outs[i].data;
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
            Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            Point classIdPoint;
            double confidence;
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if (confidence > confidenceThreshold) {
                int centerX = (int)(data[0] * frameSize.width);
                int centerY = (int)(data[1] * frameSize.height);
                int width = (int)(data[2] * frameSize.width);
                int height = (int)(data[3] * frameSize.height);
                int left = centerX - width / 2;
                int top = centerY - height / 2;

                Detection detection;
                detection.box = Rect(left, top, width, height);
                
                detection.class_id = classIdPoint.x;
                detection.className = classNames[classIdPoint.x];
                detection.confidence = confidence;
                detection.color = { 255, 0, 0 };
                detections.push_back(detection);

                confidences.push_back(detection.confidence);
                boxes.push_back(detection.box);
            }
        }
    }
    if(m_applyNMS == true){
        // Apply non-maximum suppression
        vector<int> indices;

        NMSBoxes(boxes, confidences, confidenceThreshold, nmsThreshold, indices);

        vector<Detection> filteredDetections;
        for (int idx : indices) {
            filteredDetections.push_back(detections[idx]);
        }
        detections = filteredDetections;
    }
}

void YOLOv2Detector::loadClassNames(const string& classNamesFile) {
    ifstream file(classNamesFile);
    if (file.is_open()) {
        string className;
        while (getline(file, className)) {
            classNames.push_back(className);
        }
        file.close();
    }
}

// void Drawer::drawDetections(const vector<Detection>& detections, Mat& frame) {
//     for (const auto& detection : detections) {
//         rectangle(frame, detection.box, detection.color, 2);
//         string label = detection.className + ": " + to_string(detection.confidence);
//         putText(frame, label, Point(detection.box.x, detection.box.y - 5), FONT_HERSHEY_SIMPLEX, 0.5, detection.color, 2);
//     }
// }

void Drawer::drawDetection(const Detection& detection, Mat& frame) {
    rectangle(frame, detection.box, detection.color, 2);
    string label = detection.className + ": " + to_string(detection.confidence);
    putText(frame, label, Point(detection.box.x, detection.box.y - 5), FONT_HERSHEY_SIMPLEX, 0.5, detection.color, 2);
}

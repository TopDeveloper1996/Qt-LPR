#pragma once
#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace cv;
using namespace dnn;
using namespace std;

struct Detection
{
    int class_id{ 0 };
    std::string className{""};
    float confidence{ 0.0 };
    cv::Scalar color{};
    cv::Rect box{};
};

class YOLOv2Detector {
public:
    YOLOv2Detector(const string& cfgFile, const string& weightsFile, const string& classNamesFile, const bool applyNMS, Size size, float conf_thresh, float nms_thresh);
    vector<Detection> detect(const Mat& frame);

private:
    Net net;
    vector<string> classNames;
    vector<string> outputLayerNames;
    void postprocess(const Mat& frame, const vector<Mat>& outs, vector<Detection>& detections);
    void loadClassNames(const string& classNamesFile);

    bool m_applyNMS;
    Size m_size;
    float confidenceThreshold;
    float nmsThreshold;
};

class Drawer {
public:
    // static void drawDetections(const vector<Detection>& detections, Mat& frame);
    static void drawDetection(const Detection& detection, Mat& frame);
};

#endif // YOLO_DETECTOR_H

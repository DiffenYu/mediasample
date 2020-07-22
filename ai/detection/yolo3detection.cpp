#include <gflags/gflags.h>
#include <glog/logging.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;
using namespace cv::dnn;

DEFINE_string(mode, "cpu", "dnn backend(cpu,cpuie,gpu)");
DEFINE_string(input, "./sample_video.mp4", "input video");
DEFINE_int32(cameraid, -1, "camera id");
DEFINE_int32(num_frames, 10, "number of frames to test");
DEFINE_bool(show, false, "whether to show or not the result");
DEFINE_int32(width, 416, "net input width");
DEFINE_int32(height, 416, "net input height");
DEFINE_double(confThreshold, 0.5, "Confidence threshold");
DEFINE_double(nmsThreshold, 0.4, "nms threshold");

float confThreshold = 0;
float nmsThreshold = 0;
vector<string> classes;

vector<String> getOutputsNames(const Net& net)
{
    static vector<String> names;
    if (names.empty()) {
        // Get the indices of the output layers, i.e. the layers with unconnected outputs
        vector<int> outLayers = net.getUnconnectedOutLayers();

        // get the names of all the layers in the network
        vector<String> layersNames = net.getLayerNames();

        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    // Draw a rectangle displaying the bounding box
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(255, 178, 50), 3);

    // Get the label for the class name and its confidence
    string label = format("%.2f", conf);
    if (!classes.empty()) {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ":" + label;
    }

    // Display the label at the top of the bounding box
    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - round(1.5 * labelSize.height)), Point(left + round(1.5 * labelSize.width), top + baseLine), Scalar(255, 255, 255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 0), 1);
}

void postprocess(Mat& frame, const vector<Mat>& outs)
{
    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boxes;

    for (size_t i = 0; i < outs.size(); i++) {
        float* data = (float*)outs[i].data;
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
            Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            Point classIdPoint;
            double confidence;
            minMaxLoc(scores, 0, &confidence, &classIdPoint);
            if (confidence > confThreshold) {
                int centerX = (int)(data[0] * frame.cols);
                int centerY = (int)(data[1] * frame.rows);
                int width = (int)(data[2] * frame.cols);
                int height = (int)(data[3] * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;

                classIds.push_back(classIdPoint.x);
                confidences.push_back((float)confidence);
                boxes.push_back(Rect(left, top, width, height));
            }
        }
    }

    vector<int> indices;
    NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    for (size_t i = 0; i < indices.size(); ++i) {
        int idx = indices[i];
        Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y, box.x + box.width, box.y + box.height, frame);
    }
}

int main(int argc, char** argv)
{
    LOG(INFO) << "USAGE : ./yolo3detection --mode=[cpu|gpu|cpuie] --input=<videofile> ";
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging("YOLO3Detection");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    string classesFile = "coco.names";
    ifstream ifs(classesFile.c_str());
    string line;
    while (getline(ifs, line)) classes.push_back(line);

    int inWidth = FLAGS_width;
    int inHeight = FLAGS_height;
    confThreshold = FLAGS_confThreshold;
    nmsThreshold = FLAGS_nmsThreshold;

    cv::VideoCapture cap(FLAGS_input);
    if (FLAGS_cameraid != -1) {
        cap.release();
        cap = cv::VideoCapture(FLAGS_cameraid);
        LOG(INFO) << "Use camera " << FLAGS_cameraid << " as input!";
    }

    if (!cap.isOpened()) {
        LOG(ERROR) << "Unable to open the video capture";
        return 1;
    }

    Mat frame, frameCopy;
    int frameWidth = cap.get(CAP_PROP_FRAME_WIDTH);
    int frameHeight = cap.get(CAP_PROP_FRAME_HEIGHT);

    String modelConfig = "yolov3.cfg";
    String modelWeights = "yolov3.weights";

    Net net = readNetFromDarknet(modelConfig, modelWeights);
    net.setPreferableBackend(DNN_BACKEND_DEFAULT);
    net.setPreferableTarget(DNN_TARGET_CPU);

    while (waitKey(1) < 0) {
        cap >> frame;
        Mat inpBlob = blobFromImage(frame, 1.0 / 255, Size(inWidth, inHeight), Scalar(0, 0, 0), false, false);
        net.setInput(inpBlob);
        vector<Mat> outputs;
        net.forward(outputs, getOutputsNames(net));
        postprocess(frame, outputs);
        if (FLAGS_show) {
            imshow("Yolov3 Detection", frame);
        }
    }

    // When everything done, release the video capture and write object
    cap.release();
    gflags::ShutDownCommandLineFlags();

    return 0;
}

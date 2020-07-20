#include <gflags/gflags.h>
#include <glog/logging.h>
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
DEFINE_int32(width, 656, "net input width");
DEFINE_int32(height, 368, "net input height");
DEFINE_double(thresh, 0.05, "net thresh");

#define COCO

#ifdef MPI
const int POSE_PAIRS[14][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 1, 5 },
    { 5, 6 }, { 6, 7 }, { 1, 14 }, { 14, 8 }, { 8, 9 },
    { 9, 10 }, { 14, 11 }, { 11, 12 }, { 12, 13 } };

string protoFile = "pose/mpi/pose_deploy_linevec_faster_4_stages.prototxt";
string weightsFile = "pose/mpi/pose_iter_160000.caffemodel";

int nPoints = 15;
#endif

#ifdef COCO
const int POSE_PAIRS[17][2] = { { 1, 2 }, { 1, 5 }, { 2, 3 }, { 3, 4 }, { 5, 6 },
    { 6, 7 }, { 1, 8 }, { 8, 9 }, { 9, 10 }, { 1, 11 },
    { 11, 12 }, { 12, 13 }, { 1, 0 }, { 0, 14 }, { 14, 16 },
    { 0, 15 }, { 15, 17 } };

string protoFile = "pose/coco/pose_deploy_linevec.prototxt";
string weightsFile = "pose/coco/pose_iter_440000.caffemodel";

int nPoints = 18;
#endif

int main(int argc, char** argv)
{
    LOG(INFO) << "USAGE : ./openpose --mode=[cpu|gpu|cpuie] --input=<videofile> ";
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging("OPENPOSE");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    int inWidth = FLAGS_width;
    int inHeight = FLAGS_height;
    double thresh = FLAGS_thresh;

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

    VideoWriter video("Output-Skeleton.avi",
        VideoWriter::fourcc('M', 'J', 'P', 'G'), 10,
        Size(frameWidth, frameHeight));

    Net net = readNetFromCaffe(protoFile, weightsFile);

    if (FLAGS_mode == "cpu") {
        LOG(INFO) << "use CPU backend";
        net.setPreferableBackend(DNN_BACKEND_DEFAULT);
        net.setPreferableTarget(DNN_TARGET_CPU);
    } else if (FLAGS_mode == "gpu") {
        LOG(INFO) << "use GPU backend";
        net.setPreferableBackend(DNN_BACKEND_CUDA);
        net.setPreferableTarget(DNN_TARGET_CUDA);
    } else if (FLAGS_mode == "cpuie") {
        LOG(INFO) << "use CPUIE backend";
        net.setPreferableBackend(DNN_BACKEND_INFERENCE_ENGINE);
        net.setPreferableTarget(DNN_TARGET_CPU);
    }

    long long frame_num = 0;
    double sum_perf = 0;
    int index = 0;

    double t = 0;
    while (waitKey(1) < 0) {
        double t = (double)cv::getTickCount();

        cap >> frame;
        frameCopy = frame.clone();
        Mat inpBlob = blobFromImage(frame, 1.0 / 255, Size(inWidth, inHeight),
            Scalar(0, 0, 0), false, false);

        net.setInput(inpBlob);

        double perf = (double)cv::getTickCount();
        Mat output = net.forward();
        perf = ((double)cv::getTickCount() - perf) / cv::getTickFrequency();
        index++;
        if (index >= 5) { // the first five frames used to warm up
            sum_perf += perf;
            frame_num++;
            LOG(INFO) << "frame: " << frame_num << ", time: " << perf;
            if (frame_num == FLAGS_num_frames)
                break;
        }
        // vector<double> layersTimings;
        // double tick_freq = getTickFrequency();
        // double time_ms = net.getPerfProfile(layersTimings) / tick_freq * 1000;
        // for (auto i : layersTimings) {
        // std::cout << i << " " << std::endl;
        //}

        int H = output.size[2];
        int W = output.size[3];

        // find the position of the body parts
        vector<Point> points(nPoints);
        for (int n = 0; n < nPoints; n++) {
            // Probability map of corresponding body's part.
            Mat probMap(H, W, CV_32F, output.ptr(0, n));

            Point2f p(-1, -1);
            Point maxLoc;
            double prob;
            minMaxLoc(probMap, 0, &prob, 0, &maxLoc);
            if (prob > thresh) {
                p = maxLoc;
                p.x *= (float)frameWidth / W;
                p.y *= (float)frameHeight / H;

                circle(frameCopy, cv::Point((int)p.x, (int)p.y), 8, Scalar(0, 255, 255),
                    -1);
                cv::putText(frameCopy, cv::format("%d", n),
                    cv::Point((int)p.x, (int)p.y), cv::FONT_HERSHEY_COMPLEX,
                    1.1, cv::Scalar(0, 0, 255), 2);
            }
            points[n] = p;
        }

        int nPairs = sizeof(POSE_PAIRS) / sizeof(POSE_PAIRS[0]);

        for (int n = 0; n < nPairs; n++) {
            // lookup 2 connected body/hand parts
            Point2f partA = points[POSE_PAIRS[n][0]];
            Point2f partB = points[POSE_PAIRS[n][1]];

            if (partA.x <= 0 || partA.y <= 0 || partB.x <= 0 || partB.y <= 0)
                continue;

            line(frame, partA, partB, Scalar(0, 255, 255), 8);
            circle(frame, partA, 8, Scalar(0, 0, 255), -1);
            circle(frame, partB, 8, Scalar(0, 0, 255), -1);
        }

        t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
        if (FLAGS_show == true) {
            cv::putText(frame, cv::format("time taken = %.2f sec", t),
                cv::Point(50, 50), cv::FONT_HERSHEY_COMPLEX, .8,
                cv::Scalar(255, 50, 0), 2);
            imshow("Output-Keypoints", frameCopy);
            imshow("Output-Skeleton", frame);
            video.write(frame);
        }
    }

    LOG(INFO) << "sum_perf: " << sum_perf;
    LOG(INFO) << "frame_num: " << frame_num;
    LOG(INFO) << "fps: " << frame_num / sum_perf;

    // When everything done, release the video capture and write object
    cap.release();
    video.release();

    gflags::ShutDownCommandLineFlags();

    return 0;
}

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>

using namespace std;
using namespace cv;


// Convert to string
#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

DEFINE_string(input, "./sample_video.mp4", "input video");
DEFINE_int32(cameraid, -1, "camera id");
DEFINE_int32(num_frames, 10, "number of frames to test");
DEFINE_bool(show, false, "whether to show or not the result");
DEFINE_int32(width, 656, "net input width");
DEFINE_int32(height, 368, "net input height");

int main(int argc, char** argv)
{
    LOG(INFO) << "USAGE : ./tracking --input=<videofile> ";
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging("TRACKING");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    string trackerTypes[8] = { "BOOSTING", "MIL", "KCF", "TLD", "MEDIANFLOW", "GOTURN", "MOSSE", "CSRT" };

    string trackerType = trackerTypes[2];
    Ptr<Tracker> tracker;
#if (CV_MAJOR_VERSION == 3 && CV_MINOR_VERSION < 3)
    {
        tracker = Tracker::create(trackerType);
    }
#else
    {
        if (trackerType == "BOOSTING")
            tracker = TrackerBoosting::create();
        if (trackerType == "MIL")
            tracker = TrackerMIL::create();
        if (trackerType == "KCF")
            tracker = TrackerKCF::create();
        if (trackerType == "TLD")
            tracker = TrackerTLD::create();
        if (trackerType == "MEDIANFLOW")
            tracker = TrackerMedianFlow::create();
        if (trackerType == "GOTURN")
            tracker = TrackerGOTURN::create();
        if (trackerType == "MOSSE")
            tracker = TrackerMOSSE::create();
        if (trackerType == "CSRT")
            tracker = TrackerCSRT::create();
    }
#endif

    cv::VideoCapture cap(FLAGS_input);
    //if (FLAGS_cameraid != -1) {
    //cap.release();
    //cap = cv::VideoCapture(FLAGS_cameraid);
    //LOG(INFO) << "Use camera " << FLAGS_cameraid << " as input!";
    //}

    LOG(INFO) << "FLAGS_input: " << FLAGS_input;

    if (!cap.isOpened()) {
        LOG(ERROR) << "Unable to open the video capture";
        return 1;
    }

    Mat frame;
    bool ok = cap.read(frame);
    Rect2d bbox(287, 23, 86, 320);
    rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
    imshow("Tracking", frame);
    tracker->init(frame, bbox);

    while (cap.read(frame)) {
        // Start timer
        double timer = (double)getTickCount();

        // Update the tracking result
        bool ok = tracker->update(frame, bbox);

        // Calculate Frames per second (FPS)
        float fps = getTickFrequency() / ((double)getTickCount() - timer);

        if (ok) {
            // Tracking success : Draw the tracked object
            rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
        } else {
            // Tracking failure detected.
            putText(frame, "Tracking failure detected", Point(100, 80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 255), 2);
        }

        // Display tracker type on frame
        putText(frame, trackerType + " Tracker", Point(100, 20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50, 170, 50), 2);

        // Display FPS on frame
        putText(frame, "FPS : " + SSTR(int(fps)), Point(100, 50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50, 170, 50), 2);

        // Display frame.
        imshow("Tracking", frame);

        // Exit if ESC pressed.
        int k = waitKey(1);
        if (k == 27) {
            break;
        }
    }

    cap.release();
    gflags::ShutDownCommandLineFlags();

    return 0;
}

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>

using namespace cv;
using namespace std;

#define USE_GSTREAMER 1

// See www.asciitable.com
#define ESCAPE_KEY (27)
#define SYSTEM_ERROR (-1)

// gStreamer parameters
#define CAPTURE_WIDTH 1280 // 1280
#define CAPTURE_HEIGHT 720 // 720
#define DISPLAY_WIDTH CAPTURE_WIDTH
#define DISPLAY_HEIGHT CAPTURE_HEIGHT
#define FRAMERATE 60.0
#define FLIP_MODE 0


/**
 * @brief this is needed to load a CSI camera configurations via gStreamer.
 * source: https://github.com/JetsonHacksNano/CSI-Camera/blob/master/simple_camera.cpp
 *
 * @param capture_width width of captured frame.
 * @param capture_height height of captured frame.
 * @param display_width width of frame to be displayed.
 * @param display_height width of frame to be displayed.
 * @param framerate frame rate of stream.
 * @param flip_method flip frame if necessary, 90 deg intervals.
 *                    (0 = 0 deg, 1 = 90 deg, 2 = 180 deg, 3 = 270 deg).
 * @return std::string a string to be passed as gStreamer command line arguments.
 */
static std::string gstreamer_pipeline(int capture_width, int capture_height,
                                      int display_width, int display_height,
                                      int framerate, int flip_method) {

    return "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" +
           std::to_string(capture_width) + ", height=(int)" +
           std::to_string(capture_height) + ", framerate=(fraction)" +
           std::to_string(framerate) +
           "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) +
           " ! video/x-raw, width=(int)" + std::to_string(display_width) +
           ", height=(int)" + std::to_string(display_height) +
           ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! "
           "appsink";
}


/**
 * @brief initialize and test camera stream.
 *
 * @param cam_stream VideoCapture object to tie it to the camera device.
 * @return true initialized and tested successfully.
 * @return false failed to initialize camera stream.
 */
static bool init_camera_stream(cv::VideoCapture &cam_stream) {

    cv::Mat temp_frame;
    bool    ok = false;

#if USE_GSTREAMER
    // initialize video stream from a CSI camera device
    std::string pipeline =
        gstreamer_pipeline(CAPTURE_WIDTH, CAPTURE_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                           FRAMERATE, FLIP_MODE);

    cam_stream.open(pipeline, cv::CAP_GSTREAMER);
#else
    // initialize video stream from a USB camera device
    cam_stream.open(0);
#endif /* USE_GSTREAMER */

    // ensure stream is open, and grab one frame to make sure it's not empty
    if (cam_stream.isOpened()) {
        cam_stream >> temp_frame;
        if (!temp_frame.empty()) { ok = true; }
    } else {
        ok = false;
    }

    // handle error if any
    // if (!ok) { handle_error(error_failed_cam_init); }

    return ok;
}


/**
 * @brief entry point.
 *
 * @return int
 */
int main(void) {
    cv::Mat     src_frame;        // source frames
    cv::namedWindow("video_display");
    char winInput;

    // init camera
    std::cout << "initializing...";
    cv::VideoCapture cam_stream;
    if (!init_camera_stream(cam_stream)) {
        std::cerr << "[FAILED]" << std::endl;
        exit(-1);
    }

    // camera stream now established. go wild.
    while(1)
    {
        cam_stream >> src_frame;

        imshow("video display", src_frame);

        if((winInput = waitKey(10) == ESCAPE_KEY))
        {
            break;
        }
        else
        {
            cout << "input" << winInput << "ignored" << endl;
        }
    }
 
    destroyWindow("vidoe_display");

    return 0;
}


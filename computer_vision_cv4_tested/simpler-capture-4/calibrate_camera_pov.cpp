/**
 * @file calibrate_camera_pov.cpp
 * @author Feras Alshehri (falshehri@mail.csuchico.edu)
 * @brief Use to calibrate the point of view of camera. 
 * @version 0.1
 * @date 2022-09-30
 *
 * @build_with:
 * g++ calibrate_camera_pov.cpp -o calibrate_camera_pov `pkg-config --libs opencv4`
 * -I/usr/include/opencv4/
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <time.h>

#define WIN_TITLE "TEST"
#define ESC_ASCII 27

#define NUM_OF_FRAMES 1300
#define USE_GSTREAMER 1
// gStreamer parameters
#define CAPTURE_WIDTH 1280 // 1280
#define CAPTURE_HEIGHT 720 // 720
#define DISPLAY_WIDTH CAPTURE_WIDTH
#define DISPLAY_HEIGHT CAPTURE_HEIGHT
#define FRAMERATE 60.0
#define FLIP_MODE 0

/**
 * @brief Get timestamp in seconds and nanoseconds (epoch)
 *
 * @return std::string epoch time in seconds and nano seconds (s_ns)
 */
std::string get_time_ns(std::string sep = "_") {
    struct timespec curr_time_ns = {};

    clock_gettime(CLOCK_REALTIME, &curr_time_ns);

    return std::to_string(curr_time_ns.tv_sec) + sep +
           std::to_string(curr_time_ns.tv_nsec);
}

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

int main(void) {
    cv::Mat      src_frame; // source frames
    long double  t_i;
    long double  t_f;
    unsigned int frames_counter = 0;
    int          user_input     = 0;

    // init camera
    std::cout << "initializing...";
    cv::VideoCapture cam_stream;
    if (!init_camera_stream(cam_stream)) {
        std::cerr << "[FAILED]" << std::endl;
        exit(-1);
    }

    // test camera
    std::cout << "[OK]" << std::endl;

    // create a window to display our video
    cv::namedWindow(WIN_TITLE);

    t_i = std::stold(get_time_ns("."));

    while (user_input != ESC_ASCII) {

        // capture frame from stream
        cam_stream >> src_frame;

        t_f = std::stold(get_time_ns("."));

        // calculate fps
        if (t_f - t_i < 1.0) {
            ++frames_counter;
        } else {
            // @TODO: find a better way too show FPS instead of spamming stdout
            std::cout << "fps=" << frames_counter << "(over " << t_f - t_i << " s)"
                      << std::endl;
            frames_counter = 0;
            t_i            = t_f;
        }

        // show frame
        cv::imshow(WIN_TITLE, src_frame);

        // check for user input
        user_input = cv::waitKey(5);
    }

    // cleanup
    cv::destroyWindow(WIN_TITLE);
    cam_stream.release();
}
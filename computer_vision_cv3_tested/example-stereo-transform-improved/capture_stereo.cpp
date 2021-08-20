/*
 *
 *  Example by Sam Siewert  - modified for dual USB Camera capture to use to
 *                            experiment and learn Stereo vision concepts pairing with
 *                            "Learning OpenCV" by Gary Bradski and Adrian Kaehler
 *
 *  For more advanced stereo vision processing for camera calibration, disparity, and
 *  point-cloud generation from a left and right image, see samples directory where you
 *  downloaded and built Opencv latest: e.g. opencv-2.4.7/samples/cpp/stero_match.cpp
 *
 *  ADDED:
 *
 *  1) 4th argument of "t" for transform [Canny and Hough Linear] or "d" for disparity
 *
 *  2) 4th argument is "h" for Hough Linear transform and otherwise "c" for Canny
 *
 *  NOTE: Uncompressed YUV at 640x480 for 2 cameras is likely to exceed
 *        your USB 2.0 bandwidth available.  The calculation is:
 *        2 cameras x 640 x 480 x 2 bytes_per_pixel x 30 Hz = 36000 KBytes/sec
 *        
 *        About 370 Mbps (assuming 8b/10b link encoding), and USB 2.0 is 480
 *        Mbps at line rate with no overhead.
 *
 *        So, for full performance with 2 cameras, drop resolution down to 320x240.
 *
 *        With a single camera I had no problem running 1280x720 (720p). 
 *
 *        I tested with really old Logitech C200 and newer C270 webcams, both are well
 *        supported and tested with the Linux UVC driver - http://www.ideasonboard.org/uvc
 *
 *        Use with a native Linux installation and any UVC driver camera.  You can
 *        verify that UVC is loaded with "lsmod | grep uvc" after you plug in your
 *        camera (modprobe if you need to) and you can verify a USB camera's link wiht
 *        "lsusb" and "dmesg" after plugging it in.
 *        
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/contrib/contrib.hpp"

using namespace cv;
using namespace std;

// If you have enough USB 2.0 bandwidth, then run at higher resolution
//#define HRES_COLS (1280)
//#define VRES_ROWS (720)
#define HRES_COLS (640)
#define VRES_ROWS (480)

// Should always work for uncompressed USB 2.0 dual cameras
//#define HRES_COLS (320)
//#define VRES_ROWS (240)

#define ESC_KEY (27)

char snapshotname[80]="snapshot_xxx.jpg";

int main( int argc, char** argv )
{
    double prev_frame_time, prev_frame_time_l, prev_frame_time_r;
    double curr_frame_time, curr_frame_time_l, curr_frame_time_r;
    struct timespec frame_time, frame_time_l, frame_time_r;
    CvCapture *capture;
    CvCapture *capture_l;
    CvCapture *capture_r;
    IplImage *frame, *frame_l, *frame_r;
    int dev=0, devl=0, devr=1;

    int computeDisparity=0;
    int applyCannyTransform=0;
    int applyHoughTransform=0;

    Mat gray_l, canny_frame_l;
    vector<Vec4i> lines_l;
    Mat gray_r, canny_frame_r;
    vector<Vec4i> lines_r;


    Mat disp;

    StereoVar myStereoVar;


    if(argc == 1)
    {
        printf("Will open DEFAULT video device video0\n");
        capture = cvCreateCameraCapture(0);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES_COLS);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES_ROWS);
    }

    if(argc == 2)
    {
        printf("argv[1]=%s\n", argv[1]);
        sscanf(argv[1], "%d", &dev);
        printf("Will open SINGLE video device %d\n", dev);
        capture = cvCreateCameraCapture(dev);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES_COLS);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES_ROWS);
    }

    if(argc >= 3)
    {

        printf("argv[1]=%s, argv[2]=%s\n", argv[1], argv[2]);
        sscanf(argv[1], "%d", &devl);
        sscanf(argv[2], "%d", &devr);
        printf("Will open DUAL video devices %d and %d\n", devl, devr);
        capture_l = cvCreateCameraCapture(devl);
        capture_r = cvCreateCameraCapture(devr);
        cvSetCaptureProperty(capture_l, CV_CAP_PROP_FRAME_WIDTH, HRES_COLS);
        cvSetCaptureProperty(capture_l, CV_CAP_PROP_FRAME_HEIGHT, VRES_ROWS);
        cvSetCaptureProperty(capture_r, CV_CAP_PROP_FRAME_WIDTH, HRES_COLS);
        cvSetCaptureProperty(capture_r, CV_CAP_PROP_FRAME_HEIGHT, VRES_ROWS);

        if(argc == 4)
        {

            if(strncmp(argv[3],"d",1) == 0)
            {
                computeDisparity=1;
                // set parameters for disparity
                myStereoVar.levels = 3;
                myStereoVar.pyrScale = 0.5;
                myStereoVar.nIt = 25;
                myStereoVar.minDisp = -16;
                myStereoVar.maxDisp = 0;
                myStereoVar.poly_n = 3;
                myStereoVar.poly_sigma = 0.0;
                myStereoVar.fi = 15.0f;
                myStereoVar.lambda = 0.03f;
                myStereoVar.penalization = myStereoVar.PENALIZATION_TICHONOV;
                myStereoVar.cycle = myStereoVar.CYCLE_V;
                myStereoVar.flags = myStereoVar.USE_SMART_ID 
                                  | myStereoVar.USE_AUTO_PARAMS 
                                  | myStereoVar.USE_INITIAL_DISPARITY 
                                  | myStereoVar.USE_MEDIAN_FILTERING ;

                cvNamedWindow("Capture LEFT", CV_WINDOW_AUTOSIZE);
                cvNamedWindow("Capture RIGHT", CV_WINDOW_AUTOSIZE);
                namedWindow("Capture DISPARITY", CV_WINDOW_AUTOSIZE);
            }
            else if(strncmp(argv[3],"h",1) == 0)
            {
                applyHoughTransform=1;
            }
            else
            {
                applyCannyTransform=1;
            }
        }

        while(1)
        {
            frame_l=cvQueryFrame(capture_l);
            frame_r=cvQueryFrame(capture_r);

            if(!frame_l) break;
            if(!frame_r) break;
  
            if(applyCannyTransform || applyHoughTransform)
            {
                //Mat mat_frame_l(frame_l);
                Mat mat_frame_l(cvarrToMat(frame_l));
                Canny(mat_frame_l, canny_frame_l, 50, 200, 3);
                //Mat mat_frame_r(frame_r);
                Mat mat_frame_r(cvarrToMat(frame_r));
                Canny(mat_frame_r, canny_frame_r, 50, 200, 3);

                if(applyHoughTransform)
                {
                    HoughLinesP(canny_frame_l, lines_l, 1, CV_PI/180, 50, 50, 10);
                    HoughLinesP(canny_frame_r, lines_r, 1, CV_PI/180, 50, 50, 10);

                    for( size_t i = 0; i < lines_l.size(); i++ )
                    {
                      Vec4i l = lines_l[i];
                      line(mat_frame_l, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
                    }

                    for( size_t i = 0; i < lines_r.size(); i++ )
                    {
                      Vec4i l = lines_r[i];
                      line(mat_frame_r, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
                    }
                }

            }
            else if(computeDisparity)
            {
                //Mat mat_frame_l(frame_l);
                //Mat mat_frame_r(frame_r);
                Mat mat_frame_l(cvarrToMat(frame_l));
                Mat mat_frame_r(cvarrToMat(frame_r));
                myStereoVar(Mat(frame_l, 0), Mat(frame_r, 0), disp);
            }
 
            if(!frame_l) break;
            else
            {
                clock_gettime(CLOCK_REALTIME, &frame_time_l);
                curr_frame_time_l=((double)frame_time_l.tv_sec * 1000.0) + 
                                  ((double)((double)frame_time_l.tv_nsec / 1000000.0));
            }
            if(!frame_r) break;
            else
            {
                clock_gettime(CLOCK_REALTIME, &frame_time_r);
                curr_frame_time_r=((double)frame_time_r.tv_sec * 1000.0) + 
                                  ((double)((double)frame_time_r.tv_nsec / 1000000.0));
            }

            if(applyCannyTransform)
            {
                 
                IplImage canny_img_l  = canny_frame_l.operator IplImage();
                IplImage canny_img_r  = canny_frame_r.operator IplImage();

                cvShowImage("Capture LEFT", &canny_img_l);
                cvShowImage("Capture RIGHT", &canny_img_r);
            }
            else
            {
                cvShowImage("Capture LEFT", frame_l);
                cvShowImage("Capture RIGHT", frame_r);
            }

            if(computeDisparity)
                imshow("Capture DISPARITY", disp);

            printf("LEFT dt=%5.2lf msec, RIGHT dt=%5.2lf msec\n", 
                   (curr_frame_time_l - prev_frame_time_l),
                   (curr_frame_time_r - prev_frame_time_r));


            // Set to pace frame display and capture rate
            char c = cvWaitKey(10);
            if(c == ESC_KEY)
            {
                sprintf(&snapshotname[9], "left_%8.4lf.jpg", curr_frame_time_l);
                cvSaveImage(snapshotname, frame_l);
                sprintf(&snapshotname[9], "right_%8.4lf.jpg", curr_frame_time_r);
                cvSaveImage(snapshotname, frame_r);
            }
            else if((c == 'q') || (c == 'Q'))
            {
                printf("Exiting ...\n");
                cvReleaseCapture(&capture_l);
                cvReleaseCapture(&capture_r);
                exit(0);
            }


            prev_frame_time_l=curr_frame_time_l;
            prev_frame_time_r=curr_frame_time_r;
        }

        cvReleaseCapture(&capture_l);
        cvReleaseCapture(&capture_r);
        cvDestroyWindow("Capture LEFT");
        cvDestroyWindow("Capture RIGHT");
    }

    else
    {
        // used to compute running averages for single camera frame rates
        double ave_framedt=0.0, ave_frame_rate=0.0, fc=0.0, framedt=0.0;
        unsigned int frame_count=0;

        cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);

        while(1)
        {
            //if(cvGrabFrame(capture)) frame=cvRetrieveFrame(capture);
            frame=cvQueryFrame(capture); // short for grab and retrieve
     
            if(!frame) break;

            else
            {
                clock_gettime(CLOCK_REALTIME, &frame_time);
                curr_frame_time=((double)frame_time.tv_sec * 1000.0) + 
                                ((double)((double)frame_time.tv_nsec / 1000000.0));
                frame_count++;

                if(frame_count > 2)
                {
                    fc=(double)frame_count;
                    ave_framedt=((fc-1.0)*ave_framedt + framedt)/fc;
                    ave_frame_rate=1.0/(ave_framedt/1000.0);
                }
            }

            cvShowImage("Capture Example", frame);
            printf("Frame @ %u sec, %lu nsec, dt=%5.2lf msec, avedt=%5.2lf msec, rate=%5.2lf fps\n", 
                   (unsigned)frame_time.tv_sec, 
                   (unsigned long)frame_time.tv_nsec,
                   framedt, ave_framedt, ave_frame_rate);

            // Set to pace frame capture and display rate
            char c = cvWaitKey(10);
            if(c == ESC_KEY)
            {
                sprintf(&snapshotname[9], "%8.4lf.jpg", curr_frame_time);
                cvSaveImage(snapshotname, frame);
            }
            else if((c == 'q') || (c == 'Q'))
            {
                printf("Exiting ...\n");
                cvReleaseCapture(&capture);
                exit(0);
            }

            framedt=curr_frame_time - prev_frame_time;
            prev_frame_time=curr_frame_time;
        }

        cvReleaseCapture(&capture);
        cvDestroyWindow("Capture Example");
    }
 
};

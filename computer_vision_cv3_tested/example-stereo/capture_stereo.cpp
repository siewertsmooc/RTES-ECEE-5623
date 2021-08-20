/*
 *
 *  Example by Sam Siewert  - modified for dual USB Camera capture
 *
 *  NOTE: Uncompressed YUV at 640x480 for 2 cameras is likely to exceed
 *        your USB 2.0 bandwidth available.  The calculation is:
 *        2 cameras x 640 x 480 x 2 bytes_per_pixel x 30 Hz = 36000 KBytes/sec
 *        
 *        About 370 Mbps (assuming 8b/10b link encoding), and USB 2.0 is 480
 *        Mbps at line rate with no overhead.
 *
 *        So, for full performance, drop resolution down to 320x240.
 *
 *        I tested with really old Logitech C200 and newer C270 webcams, both are well
 *        supported and tested with the Linux UVC driver - http://www.ideasonboard.org/uvc
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
//#define HRES_COLS (640)
//#define VRES_ROWS (480)

// Should always work for uncompressed USB 2.0 dual cameras
#define HRES_COLS (320)
#define VRES_ROWS (240)

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
    int dev=0, devl=0, devr=0;

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

        while(1)
        {
            frame_l=cvQueryFrame(capture_l);
            frame_r=cvQueryFrame(capture_r);
  
            myStereoVar(Mat(frame_l, 0), Mat(frame_r, 0), disp);
 
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

            cvShowImage("Capture LEFT", frame_l);
            cvShowImage("Capture RIGHT", frame_r);
            imshow("Capture DISPARITY", disp);
            printf("LEFT dt=%lf msec, RIGHT dt=%lf\n", 
                   (curr_frame_time_l - prev_frame_time_l),
                   (curr_frame_time_r - prev_frame_time_r));


            // Set to pace frame display and capture rate
            char c = cvWaitKey(10);
            if( c == ESC_KEY )
            {
                sprintf(&snapshotname[9], "left_%8.4lf.jpg", curr_frame_time_l);
                cvSaveImage(snapshotname, frame_l);
                sprintf(&snapshotname[9], "right_%8.4lf.jpg", curr_frame_time_r);
                cvSaveImage(snapshotname, frame_r);
            }

            if( c == 'q' )
                break;

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
        cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);

        while(1)
        {
            if(cvGrabFrame(capture)) frame=cvRetrieveFrame(capture);
            // frame=cvQueryFrame(capture); // short for grab and retrieve
     
            if(!frame) break;
            else
            {
                clock_gettime(CLOCK_REALTIME, &frame_time);
                curr_frame_time=((double)frame_time.tv_sec * 1000.0) + 
                                ((double)((double)frame_time.tv_nsec / 1000000.0));
            }

            cvShowImage("Capture Example", frame);
            printf("Frame time = %u sec, %lu nsec, dt=%lf msec\n", 
                   (unsigned)frame_time.tv_sec, 
                   (unsigned long)frame_time.tv_nsec,
                   (curr_frame_time - prev_frame_time));

            // Set to pace frame capture and display rate
            char c = cvWaitKey(10);
            if( c == ESC_KEY )
            {
                sprintf(&snapshotname[9], "%8.4lf.jpg", curr_frame_time);
                cvSaveImage(snapshotname, frame);
            }

            if( c == 'q' )
                break;

            prev_frame_time=curr_frame_time;
        }

        cvReleaseCapture(&capture);
        cvDestroyWindow("Capture Example");
    }
 
};

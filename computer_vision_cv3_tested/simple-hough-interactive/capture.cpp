/*
 *
 *  Example by Sam Siewert 
 *
 *  Updated for OpenCV 3.1
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define HRES 640
#define VRES 480


int main( int argc, char** argv )
{
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    //CvCapture* capture = (CvCapture *)cvCreateCameraCapture(0);
    //CvCapture* capture = (CvCapture *)cvCreateCameraCapture(argv[1]);
    CvCapture* capture;
    IplImage* frame;
    int dev=0;
    Mat gray, canny_frame, cdst;
    vector<Vec4i> lines;

    if(argc > 1)
    {
        sscanf(argv[1], "%d", &dev);
        printf("using %s\n", argv[1]);
    }
    else if(argc == 1)
        printf("using default\n");

    else
    {
        printf("usage: capture [dev]\n");
        exit(-1);
    }

    capture = (CvCapture *)cvCreateCameraCapture(dev);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    while(1)
    {
        frame=cvQueryFrame(capture);

        Mat mat_frame(cvarrToMat(frame));
        Canny(mat_frame, canny_frame, 50, 200, 3);

        cvtColor(canny_frame, cdst, CV_GRAY2BGR);
        cvtColor(mat_frame, gray, CV_BGR2GRAY);

        HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

        for( size_t i = 0; i < lines.size(); i++ )
        {
          Vec4i l = lines[i];
          line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
        }

     
        if(!frame) break;

        // cvShowImage seems to be a problem in 3.1
        //cvShowImage("Capture Example", frame);

        imshow("Capture Example", mat_frame);

        char c = cvWaitKey(10);
        if( c == 'q' ) break;
    }

    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
};

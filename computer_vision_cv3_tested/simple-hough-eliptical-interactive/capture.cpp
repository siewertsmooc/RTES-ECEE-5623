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
    Mat gray;
    vector<Vec3f> circles;

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
        if(!frame) break;

        Mat mat_frame(cvarrToMat(frame));
        
        // Does not work in OpenCV 3.1
        //cvtColor(mat_frame, gray, CV_BGR2GRAY);
        cvtColor(mat_frame, gray, COLOR_BGR2GRAY);

        GaussianBlur(gray, gray, Size(9,9), 2, 2);

        HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 100, 50, 0, 0);

        printf("circles.size = %ld\n", circles.size());

        for( size_t i = 0; i < circles.size(); i++ )
        {
          Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
          int radius = cvRound(circles[i][2]);
          // circle center
          circle( mat_frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
          // circle outline
          circle( mat_frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
        }

        // Does not work in OpenCV 3.1
        //cvShowImage("Capture Example", frame);

        imshow("Capture Example", mat_frame);

        char c = cvWaitKey(10);
        if( c == 'q' ) break;
    }

    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
};

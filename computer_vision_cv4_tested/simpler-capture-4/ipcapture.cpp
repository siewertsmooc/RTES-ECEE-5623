/*
 *
 *  Example by Sam Siewert 
 *
 *  Updated 1/24/2022 for OpenCV 4.x for Jetson nano 2g 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

int main( int argc, char** argv )
{
    cv::VideoCapture vcap;
    cv::Mat image;

    // This works with a Microseven IP security camera
    const std::string videoStreamAddress = "rtsp://172.19.172.216/11";

    //open the video stream and make sure it's opened
    if(!vcap.open(videoStreamAddress)) {
        std::cout << "Error opening video stream or file" << std::endl;
        return -1;
    }

    for(;;) 
    {
        if(!vcap.read(image)) 
	{
            std::cout << "No frame" << std::endl;
            cv::waitKey();
        }

        cv::imshow("Output Window", image);
        if(cv::waitKey(1) >= 0) break;
    }   
};

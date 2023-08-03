/*
 *
 *  Example by Sam Siewert 
 *
 *  Created for OpenCV 4.x for Jetson Nano 2g, based upon
 *  https://docs.opencv.org/4.1.1
 *
 *  Tested with JetPack 4.6 which installs OpenCV 4.1.1
 *  (https://developer.nvidia.com/embedded/jetpack)
 *
 *  Based upon earlier simpler-capture examples created
 *  for OpenCV 2.x and 3.x (C and C++ mixed API) which show
 *  how to use OpenCV instead of lower level V4L2 API for the
 *  Linux UVC driver.
 *
 *  Verify your hardware and OS configuration with:
 *  1) lsusb
 *  2) ls -l /dev/video*
 *  3) dmesg | grep UVC
 *
 *  Note that OpenCV 4.x only supports the C++ API
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

// See www.asciitable.com
#define ESCAPE_KEY (27)
#define SYSTEM_ERROR (-1)

int main()
{
   VideoCapture cam0(0);
   namedWindow("video_display");
   char winInput;

   struct timespec curr_t;
   double curr_time, prev_time;
   unsigned int frameCnt=0;
 

   if (!cam0.isOpened())
   {
       exit(SYSTEM_ERROR);
   }

   cam0.set(CAP_PROP_FRAME_WIDTH, 640);
   cam0.set(CAP_PROP_FRAME_HEIGHT, 480);

   clock_gettime(CLOCK_MONOTONIC, &curr_t);
   curr_time = (double)curr_t.tv_sec + ((double)curr_t.tv_nsec) / 1000000000.0;
   prev_time=curr_time;

   while (1)
   {
      Mat frame;

      // get time here
      clock_gettime(CLOCK_MONOTONIC, &curr_t);
      curr_time = (double)curr_t.tv_sec + ((double)curr_t.tv_nsec) / 1000000000.0;
      printf("frameCnt=%u, curr_time=%lf sec, dt=%lf msec\n", frameCnt, curr_time, (curr_time-prev_time)*1000.0);

      cam0.read(frame);

      prev_time=curr_time;
      frameCnt++;

      imshow("video_display", frame);


      if ((winInput = waitKey(1)) == ESCAPE_KEY)
      //if ((winInput = waitKey(0)) == ESCAPE_KEY)
      {
          break;
      }
      else if(winInput == 'n')
      {
	  cout << "input " << winInput << " ignored" << endl;
      }
      
   }

   destroyWindow("video_display"); 
};

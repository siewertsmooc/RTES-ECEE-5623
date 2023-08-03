#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;


// See www.asciitable.com
#define ESCAPE_KEY (27)
#define SYSTEM_ERROR (-1)


Mat canny_frame, timg_gray, timg_grad;
Mat frame;

int lowThreshold = 0;
const int max_lowThreshold = 100;
const int ratio = 3;
const int kernel_size = 3;
const char* window_name = "Edge Map";


void CannyThreshold(int, void*)
{
    cvtColor(frame, timg_gray, COLOR_BGR2GRAY);

    /// Reduce noise with a kernel 3x3
    blur( timg_gray, canny_frame, Size(3,3) );

    /// Canny detector
    Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

    /// Using Canny's output as a mask, we display our result
    timg_grad = Scalar::all(0);

    frame.copyTo( timg_grad, canny_frame);

    imshow( window_name, timg_grad );

}



int main( int argc, char** argv )
{
   VideoCapture cam0(0);
   namedWindow("video_display");
   char winInput;

   if (!cam0.isOpened())
   {
       exit(SYSTEM_ERROR);
   }

   cam0.set(CAP_PROP_FRAME_WIDTH, 640);
   cam0.set(CAP_PROP_FRAME_HEIGHT, 480);

   while (1)
   {
      cam0.read(frame);

      imshow("video_display", frame);

      namedWindow( window_name, WINDOW_AUTOSIZE );
      createTrackbar( "Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold );

      CannyThreshold(0, 0);

      if ((winInput = waitKey(10)) == ESCAPE_KEY)
      //if ((winInput = waitKey(0)) == ESCAPE_KEY)
      {
          break;
      }
      else if(winInput == 'n')
      {
          printf("input %c is ignored\n", winInput);
      }

   }

   destroyWindow("video_display");

}

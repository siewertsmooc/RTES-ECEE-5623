#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
using namespace cv;

#define ESCAPE_KEY (27)


Mat src, src_gray;
Mat dst, detected_edges;
int lowThreshold = 0;
const int max_lowThreshold = 100;
const int ratio = 3;
const int kernel_size = 3;
const char* window_name = "Edge Map";
static void CannyThreshold(int, void*)
{
    blur( src_gray, detected_edges, Size(3,3) );
    Canny( detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size );
    dst = Scalar::all(0);
    src.copyTo( dst, detected_edges);
    imshow( window_name, dst );
}
int main( int argc, char** argv )
{
  char winInput;

  CommandLineParser parser( argc, argv, "{@input | ../data/fruits.jpg | input image}" );
  src = imread( parser.get<String>( "@input" ), IMREAD_COLOR ); // Load an image
  if( src.empty() )
  {
    std::cout << "Could not open or find the image!\n" << std::endl;
    std::cout << "Usage: " << argv[0] << " <Input image>" << std::endl;
    return -1;
  }
  dst.create( src.size(), src.type() );
  cvtColor( src, src_gray, COLOR_BGR2GRAY );
  namedWindow( window_name, WINDOW_AUTOSIZE );
  createTrackbar( "Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold );


  while(1)
  {
      CannyThreshold(0, 0);

      if ((winInput = waitKey(10)) == ESCAPE_KEY)
      {
          break;
      }
      else if(winInput == 'n')
      {
          printf("input %c is ignored\n", winInput);
      }
  }

  return 0;
}

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
using namespace cv; using namespace std;
double alpha=1.0;  int beta=10;  /* contrast and brightness control */

int main( int argc, char** argv )
{
    // Check command line arguments
    if(argc < 2) 
    {
	    printf("Usage: brighten <input-file>\n");
        exit(-1);
    }

    Mat image = imread( argv[1] ); // read in image file
    Mat new_image = Mat::zeros( image.size(), image.type() );
    std::cout<<"* Enter alpha brighten factor [1.0-3.0]: ";std::cin>>alpha;
    std::cout<<"* Enter beta contrast increase value [0-100]: "; std::cin>>beta;

    // Do the operation new_image(i,j) = alpha*image(i,j) + beta
    for( int y = 0; y < image.rows; y++ )
    {
        for( int x = 0; x < image.cols; x++ )
        { 
            for( int c = 0; c < 3; c++ )
                new_image.at<Vec3b>(y,x)[c] =
                    saturate_cast<uchar>( alpha*( image.at<Vec3b>(y,x)[c] ) + beta );
        }
    }

    namedWindow("Original Image", 1); namedWindow("New Image", 1);
    imshow("Original Image", image); imshow("New Image", new_image);
    waitKey(); return 0;
}

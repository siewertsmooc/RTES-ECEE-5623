/*
 *
 *  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  grabber NTSC cameras to acquire digital video from a source,
 *  time-stamp each frame acquired, save to a PGM or PPM file.
 *
 *  The original code adapted was open source from V4L2 API and had the
 *  following use and incorporation policy:
 * 
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 */

int v4l2_frame_acquisition_loop(char *dev_name);

int main(int argc, char **argv)
{
    char *dev_name;

    if(argc > 1)
        dev_name = argv[1];
    else
        dev_name = "/dev/video0";

    v4l2_frame_acquisition_loop(dev_name);

}

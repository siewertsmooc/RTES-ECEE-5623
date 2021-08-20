#include "lcmwrapper.h"

#include <time.h>

int main(void)
{
    //int a=146, b=18, gcf=0, lcm=0, quot=0, rem=0;
    int a=21373, b=27, gcf=0, lcm=0, quot=0, rem=0;

    struct timespec start_time, stop_time;
    double fstart_time, fstop_time;

    gcf=gcfc(a,b);
    printf("C gcf(%d, %d)=%d\n", a, b, gcf);

    quot=divide(a,b);
    rem=remain(a,b);
    printf("C div(%d, %d)=%d, rem=%d\n", a, b, quot, rem);

    quot=divide(a*b, gcfc(a,b));
    rem=remain(a*b, gcfc(a,b));
    printf("C div(%d, %d)=%d, rem=%d\n", a*b, gcfc(a,b), quot, rem);


    clock_gettime(CLOCK_MONOTONIC, &start_time);
    lcm=lcmc(a,b);
    clock_gettime(CLOCK_MONOTONIC, &stop_time);

    fstart_time = (double)start_time.tv_sec + (double)start_time.tv_nsec/100000000.0;
    fstop_time = (double)stop_time.tv_sec + (double)stop_time.tv_nsec/100000000.0;

    printf("C lcm(%d, %d)=%d took %6.9lf\n", a, b, lcm, (fstop_time - fstart_time));



}

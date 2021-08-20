#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "perflib.h"


double readTOD(void)
{
   struct timeval tv;
   double ft=0.0;

   if(gettimeofday(&tv, NULL) != 0)
   {
       perror("readTOD");
       return 0.0;
   }
   else
   {
       ft = ((double)(((double)tv.tv_sec) + (((double)tv.tv_usec) / 1000000.0)));
       //printf("tv_sec=%ld, tv_usec=%ld, sec=%lf\n", tv.tv_sec, tv.tv_usec, ft);
   }

   return ft;
}

void elapsedTODPrint(double stopTOD, double startTOD)
{
   double dt;

   if(stopTOD > startTOD)
   {
       dt = (stopTOD - startTOD);
       printf("dt=%lf\n", dt);
   }
   else
   {
       printf("WARNING: OVERFLOW\n");
       dt=0.0; 
   }
}

double elapsedTOD(double stopTOD, double startTOD)
{
   double dt;

   if(stopTOD > startTOD)
   {
       dt = (stopTOD - startTOD);
       //printf("stopTOD=%lf, startTOD=%lf, dt=%lf\n", stopTOD, startTOD, dt);
   }
   else
   {
       printf("WARNING: OVERFLOW\n");
       dt=0.0; 
   }

   return dt;
}

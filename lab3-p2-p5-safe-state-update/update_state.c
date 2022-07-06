#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

typedef struct {
   double roll;
   double pitch;
   double yaw;
   double latitude;
   double longitude;
   double altitude;
   struct timespec timestamp;
} position_state_t




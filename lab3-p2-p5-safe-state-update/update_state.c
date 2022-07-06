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
} position_state_t;

position_state_t state;

pthread_t stateUpdaterThread;
pthread_t stateReaderThread;

pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

double timestamp;

void *updateState() {

   pthread_mutex_lock(&state_mutex);

   // Update the state values and timestamp
   state.roll = ((double) rand());
   state.pitch = ((double) rand());
   state.yaw = ((double) rand());
   state.latitude = ((double) rand());
   state.longitude = ((double) rand());
   state.altitude = ((double) rand());

	clock_gettime(CLOCK_REALTIME,&(state.timestamp));
	timestamp = (double)state.timestamp.tv_sec;
	timestamp += (double)state.timestamp.tv_nsec/(double)1000000000;
   pthread_mutex_unlock(&state_mutex);

}

int main(int argc, char *argv[])
{
   srand( time(NULL) );
}

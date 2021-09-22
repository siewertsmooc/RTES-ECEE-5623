/****************************************************************************/
/* Function: taskDelay and POSIX RT clock demonstration                     */
/*                                                                          */
/* Sam Siewert - 9/24/97                                                    */
/*                                                                          */
/****************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "time.h"

#define NSEC_PER_SEC 1000000000
#define DELAY_TICKS 1

void end_delay_test(void);

static unsigned long delay_count = 0;
static struct timespec rtclk_time = {0, 0};
static struct timespec rtclk_start_time;
static struct timespec rtclk_last_time = {0, 0};
static unsigned long accum_jitter_nsec = 0;
static int delay_error = 0;
static int ticks_per_sec, tick_nsecs, delay_nsecs, delays_per_sec;

void delay_test()
{

  int i, my_tid, duration;
  int flags = 0;
  struct timespec rtclk_resolution;

  delay_count = 0;
  rtclk_time.tv_sec = 0;
  rtclk_time.tv_nsec = 0;
  accum_jitter_nsec = 0;

  if(clock_getres(CLOCK_REALTIME, &rtclk_resolution) == ERROR)
    perror("clock_getres");

  ticks_per_sec = sysClkRateGet();
  tick_nsecs = (NSEC_PER_SEC/ticks_per_sec);
  delay_nsecs = tick_nsecs*DELAY_TICKS;
  delays_per_sec = ticks_per_sec/DELAY_TICKS;
  duration = delays_per_sec * 60; /* run test for 60 seconds */

  printf("\n\nClock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs (%ld nanosecs)\n\t%ld ticks/sec; %ld msecs/tick; %ld nsecs/tick\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec, ticks_per_sec, (tick_nsecs/1000000), tick_nsecs);

  my_tid = taskIdSelf();
  if(taskPrioritySet(my_tid, 0) == ERROR)
    perror("priority set");
  else {
    printf("Task priority set to 0\n");
    fflush(stdout);
  }

  /* start time stamp */ 
  clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
  rtclk_last_time.tv_sec = rtclk_start_time.tv_sec;
  rtclk_last_time.tv_nsec = rtclk_start_time.tv_nsec;

  while(1) {

    taskDelay(DELAY_TICKS);

    delay_count++;
    clock_gettime(CLOCK_REALTIME, &rtclk_time);

    delay_error = ((((rtclk_time.tv_sec - rtclk_last_time.tv_sec)*NSEC_PER_SEC) + (rtclk_time.tv_nsec - rtclk_last_time.tv_nsec)) - delay_nsecs); 

    accum_jitter_nsec += (unsigned long)abs(delay_error);

    rtclk_last_time.tv_sec = rtclk_time.tv_sec;
    rtclk_last_time.tv_nsec = rtclk_time.tv_nsec;

    if(delay_count > duration) end_delay_test();

  }

}

void end_delay_test(void)
{
  printf("\n");
  printf("RT clock start seconds = %ld, nanoseconds = %ld\n", 
         rtclk_start_time.tv_sec, rtclk_start_time.tv_nsec);

  printf("RT clock end seconds = %ld, nanoseconds = %ld\n", 
         rtclk_time.tv_sec, rtclk_time.tv_nsec);

  printf("RT clock delta seconds = %ld, nanoseconds = %ld\n",
         (rtclk_time.tv_sec-rtclk_start_time.tv_sec),
         (rtclk_time.tv_nsec-rtclk_start_time.tv_nsec));

  printf("\n");
  printf("Delay count = %ld @ %ld nanosecs/delay\n", delay_count, (tick_nsecs*DELAY_TICKS));
  printf("Delay-based delta = %ld secs, %ld nsecs\n",
         (delay_count/delays_per_sec),
         ((delay_count%delays_per_sec)*delay_nsecs));

  printf("Accumulated nanoseconds of jitter = %ld\n", accum_jitter_nsec);
 
  exit(0);

}

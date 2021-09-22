/****************************************************************************/
/* Function: POSIX Interval timer and RT clock demonstration                */
/*                                                                          */
/* Sam Siewert - 9/24/97                                                    */
/*                                                                          */
/****************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "timers.h"
#include "time.h"

#define NSEC_PER_MICROSEC 1000
#define NSEC_PER_MSEC 1000000
#define NSEC_PER_SEC 1000000000
#define MICROSEC_PER_MSEC 1000

/* #define MY_TICK_NSECS 16666666 */
/* #define MY_TICK_NSECS 99999996 */
#define MY_TICK_NSECS 100000000

#define TICKS_PER_SEC (NSEC_PER_SEC/MY_TICK_NSECS)

void ptimer_shutdown(void);
void ptimer_test(void);

static timer_t tt_timer;
static struct itimerspec last_itime;
static struct itimerspec itime = {{0,MY_TICK_NSECS}, {0,MY_TICK_NSECS}};
static unsigned long tick_count = 0;
static struct timespec rtclk_time = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_last_time = {0, 0};
static unsigned long accum_jitter_nsec = 0;
static int tick_error = 0;

void monitor_interval_expired(timer_t timerid, int mon_tid)
{
  int prio;

  taskPriorityGet(taskIdSelf(), &prio);
  printf("handler task prio=%d\n", prio);
  fflush(stdout);
  
  printf("Time = %ld secs, %ld nsecs\n", rtclk_time.tv_sec, rtclk_time.tv_nsec);
  printf("Delta-time = %ld secs, %ld nsecs\n", (rtclk_time.tv_sec - rtclk_last_time.tv_sec), (rtclk_time.tv_nsec - rtclk_last_time.tv_nsec));
  fflush(stdout);

  tick_count++;
  clock_gettime(CLOCK_REALTIME, &rtclk_time);

  tick_error = (((rtclk_time.tv_sec - rtclk_last_time.tv_sec)*NSEC_PER_SEC) + (rtclk_time.tv_nsec - rtclk_last_time.tv_nsec)) - MY_TICK_NSECS; 
  accum_jitter_nsec += (unsigned long)abs(tick_error);

  rtclk_last_time.tv_sec = rtclk_time.tv_sec;
  rtclk_last_time.tv_nsec = rtclk_time.tv_nsec;

  if(tick_count > (TICKS_PER_SEC*30))
    ptimer_shutdown();
}

void ptimer_test(void) {

  int i, my_tid, ticks_per_sec, tick_nsecs;
  int flags = 0;
  struct timespec rtclk_resolution;

  tick_count = 0;
  rtclk_time.tv_sec = 0;
  rtclk_time.tv_nsec = 0;
  accum_jitter_nsec = 0;
  itime.it_interval.tv_sec = 0;
  itime.it_interval.tv_nsec = MY_TICK_NSECS;
  itime.it_value.tv_sec = 0;
  itime.it_value.tv_nsec = MY_TICK_NSECS;

  if(clock_getres(CLOCK_REALTIME, &rtclk_resolution) == ERROR)
    perror("clock_getres");

  ticks_per_sec = sysClkRateGet();
  tick_nsecs = (NSEC_PER_SEC/ticks_per_sec);

  printf("\n\nClock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs (%ld nanosecs)\n\t%ld ticks/sec; %ld msecs/tick; %ld nsecs/tick\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec, ticks_per_sec, (tick_nsecs/1000000), tick_nsecs);       

  my_tid = taskIdSelf();
  if(taskPrioritySet(my_tid, 0) == ERROR)
    perror("priority set");
  else {
    printf("Task priority set to 0\n");
    fflush(stdout);
  }
 
  /* set up to signal if timer expires */
  timer_create(CLOCK_REALTIME, NULL, &tt_timer);
  timer_connect(tt_timer, (void(*)()) monitor_interval_expired, my_tid);
  timer_settime(tt_timer, flags, &itime, &last_itime);

  clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
  rtclk_last_time.tv_sec = rtclk_start_time.tv_sec;
  rtclk_last_time.tv_nsec = rtclk_start_time.tv_nsec;

  /* keep tick-tock task resident */
  while(1) {
    pause();
  }

}

void ptimer_shutdown(void)
{
  int status;

  /* disable and delete timer */

  if((status = timer_cancel(tt_timer)) == ERROR)
    perror("tt_timer");

  if((status = timer_delete(tt_timer)) == ERROR)
    perror("tt_timer");
  else {
    printf("\nTick/Tock interval timer deleted, status=%d\n", status);
    fflush(stdout);
  }                                 

  printf("RT clock start seconds = %ld, nanoseconds = %ld\n", 
         rtclk_start_time.tv_sec, rtclk_start_time.tv_nsec);

  printf("RT clock end seconds = %ld, nanoseconds = %ld\n", 
         rtclk_time.tv_sec, rtclk_time.tv_nsec);

  printf("RT clock delta seconds = %ld, nanoseconds = %ld\n",
         (rtclk_time.tv_sec-rtclk_start_time.tv_sec),
         (rtclk_time.tv_nsec-rtclk_start_time.tv_nsec));

  printf("Timer tick_count = %ld @ %ld nanosecs/tick\n", tick_count, MY_TICK_NSECS);
  printf("Tick-based delta = %ld secs, %ld nsecs\n", (tick_count/TICKS_PER_SEC), (tick_count%TICKS_PER_SEC)*MY_TICK_NSECS);

  printf("Accumulated nanoseconds of jitter = %ld\n", accum_jitter_nsec);
 
  exit(0);
 
}

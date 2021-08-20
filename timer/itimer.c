/*************************************************************************************************************/
/* Function: Interval timer software "watchdog" demonstration                                                */
/*                                                                                                           */
/* Sam Siewert -  10/1/2018                                                                                  */
/*                                                                                                           */
/* References:                                                                                               */
/*                                                                                                           */
/* 1) http://mercury.pr.erau.edu/~siewerts/cec450/code/VxWorks-Examples/Basic-Feature-Examples/itimer_test.c */
/*                                                                                                           */
/*************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

static int count = 0;
static timer_t timer_1;
static struct itimerspec itime = {{1,0}, {1,0}};
static struct itimerspec last_itime;

void interval_expired(int id)
{
  int flags = 0;
  struct timespec rtclock_time;

  clock_gettime(CLOCK_REALTIME, &rtclock_time);
  count++;
  if(count % 2)
    printf("tick @ %d sec, %d nsec\n", rtclock_time.tv_sec, rtclock_time.tv_nsec);
  else
    printf("tock @ %d sec, %d nsec\n", rtclock_time.tv_sec, rtclock_time.tv_nsec);
  fflush(stdout);

  //signal(SIGALRM, (void(*)()) interval_expired);

}


void main(void) 
{

  int i;
  int flags = 0;

  /* set up to signal SIGALRM if timer expires */
  timer_create(CLOCK_REALTIME, NULL, &timer_1);

  signal(SIGALRM, (void(*)()) interval_expired);


  /* arm the wd timer */
  itime.it_interval.tv_sec = 1;
  itime.it_interval.tv_nsec = 0;
  itime.it_value.tv_sec = 1;
  itime.it_value.tv_nsec = 0;

  timer_settime(timer_1, flags, &itime, &last_itime);

  for(;;)
    pause();

}


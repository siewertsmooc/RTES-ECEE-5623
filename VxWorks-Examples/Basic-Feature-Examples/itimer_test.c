/****************************************************************************/
/* Function: Interval timer software "watchdog" demonstration                 
/*                                                                          */
/* Sam Siewert - 7/22/97                                                    */
/*                                                                          */
/****************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "timers.h"
#include "signal.h"

#define TERMINATOR 3

static int count = 0;
static timer_t timer_1;
static struct itimerspec itime = {{1,0}, {1,0}};
static struct itimerspec last_itime;

void interval_expired(int id)
{
  int flags = 0;

  count++;
  if(count % 2)
    printf("tick\n");
  else
    printf("tock\n");
  fflush(stdout);

  signal(SIGALRM, (void(*)()) interval_expired);

}


void itimer_test(void) {

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


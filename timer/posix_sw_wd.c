/***********************************************************************************************************/
/* Function: Interval timer software "watchdog" demonstration                                              */
/*                                                                                                         */
/* Sam Siewert - 10/1/2018                                                                                 */
/* Reference: i                                                                                            */
/* http://mercury.pr.erau.edu/~siewerts/cec450/code/VxWorks-Examples/Basic-Feature-Examples/posix_sw_wd.c  */
/*                                                                                                         */
/***********************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

static int monitored_tid;
static int monitor_interval_expcount = 0;
static timer_t wd_timer_1;
static struct itimerspec itime = {{2,500000000}, {2,500000000}};
static struct itimerspec last_itime;

#define TARDY_TERMINATOR 3


//void monitored (int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10);
void monitored (void);

void monitor_interval_expired(timer_t timerid, int mon_tid)
{
  monitor_interval_expcount++;

  printf("Monitor interval expired %d times for monitored thread %d, timerid=%p\n",  monitor_interval_expcount, mon_tid, timerid);

  if(monitor_interval_expcount <= TARDY_TERMINATOR) 
  {
    printf("Respawning monitored thread and resetting watchdog timer\n");

    //taskDelete(mon_tid);

    /* Spawn a monitored task to watch and respawn/terminate */
    //if ((monitored_tid = taskSpawn ("monitored", 75, 0, (1024*8),
    //   (FUNCPTR) monitored, 0,0,0,0,0,0,0,0,0,0)) == ERROR) {
    //  printf ("Error creating monitored task to signal\n");
    //  return;
    //}

    printf ("Thread to monitor has been spawned, id=%x ... resetting monitor for new taskid\n", monitored_tid);
    fflush(stdout);

    //time_connect(wd_timer_1, (void(*)()) monitor_interval_expired, monitored_tid);
  }
  else 
  {
    printf("Thread was late more than 3 times -- will be terminated!\n"); 
    //taskDelete(monitored_tid);
    // kill thread here
  }

}


void monitored (void) 
{
  int flags = 0;
  int delay;


  while(1) 
  {

    delay = rand() % 4;
   
    /* delay between 0 and 3 seconds */ 
    printf("Tasking being delayed for %d seconds\n", delay);
    sleep(delay);
   
    /* arm the timer again -- "hit the snooze button before alarm" */
    timer_settime(wd_timer_1, flags, &itime, &last_itime);

  }

}


void main(void) 
{

  int i;
  int flags = 0;
  unsigned int seed = 554317400;

  srand(seed);

  monitor_interval_expcount = 0;

  /* set up to signal SIGALRM if timer expires */
  timer_create(CLOCK_REALTIME, NULL, &wd_timer_1);

  /* Spawn a monitored task to watch and respawn/terminate */
  //if ((monitored_tid = taskSpawn ("monitored", 75, 0, (1024*8),
  //   (FUNCPTR) monitored, 0,0,0,0,0,0,0,0,0,0)) == ERROR) {
  //  printf ("Error creating monitored task to signal\n");
  //  return;
  //}
  printf ("Thread to monitor has been spawned, id=%d\n", monitored_tid);
  fflush(stdout);

  //timer_connect(wd_timer_1, (void(*)()) monitor_interval_expired, monitored_tid);

  /* arm the wd timer for 2 second interval */
  itime.it_interval.tv_sec = 2;
  itime.it_interval.tv_nsec = 500000000;
  itime.it_value.tv_sec = 2;
  itime.it_value.tv_nsec = 500000000;

  timer_settime(wd_timer_1, flags, &itime, &last_itime);

  while(1) 
  {
    if(monitor_interval_expcount > TARDY_TERMINATOR) {
      timer_delete(wd_timer_1);
      printf ("\nAll done\n");
      exit(0);
    }
    else
      pause();
  }

}


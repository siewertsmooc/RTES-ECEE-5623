/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/*                                                                          */
/* Updated 7/28/2022 to demonstrate high resolution clock features and use  */
/*                   double precision float timestamps                      */
/*                                                                          */
/* Variation to test use of the interval timer (PIT abstration) instead of  */
/* use of a delay (nanosleep) for sequencing use.                           */
/*                                                                          */
/****************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <syslog.h>
#include <sys/time.h>
#include <sys/times.h>

#include <signal.h>

#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)

int abort_test=0;
int test_iterations=10;
unsigned int duration_sec=1, duration_nsec=0;
double duration_target_time;
static struct timespec last_pit_time = {0, 0};

static timer_t timer_1;
static struct itimerspec itime = {{1,0}, {1,0}};
static struct itimerspec last_itime;
static long pit_tick_start;

pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;



void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_OTHER\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}



double delta_t(struct timespec *stop, struct timespec *start)
{
  double dt;
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  dt = (double)dt_sec + ((double)dt_nsec/1000000000.0);

  return(dt);
}




void delay_test(void)
{
  long pit_ticks;
  static long pit_ticks_last;
  double pit_ftime, dt, err;
  struct tms systime;
  struct timespec pit_time = {0, 0};
  int flags=0;


  /* PIT: Current time for PIT handler */ 
  clock_gettime(CLOCK_REALTIME, &pit_time);
  pit_ticks=times(&systime);

  pit_ftime = (double)pit_time.tv_sec + ((double)pit_time.tv_nsec/1000000000.0);

  dt = delta_t(&pit_time, &last_pit_time);
  err = (duration_target_time - dt)*1000000.0; // err in usecs

  printf("PIT RT clock = %lf secs, ticks = %ld, dt = %lf, err = %lf usec\n", pit_ftime, (pit_ticks - pit_ticks_last), dt, err);
  syslog(LOG_CRIT, "PIT RT clock = %lf secs, ticks = %ld, dt = %lf, err = %lf usec\n", pit_ftime, (pit_ticks - pit_ticks_last), dt, err);

  test_iterations = test_iterations - 1;
  last_pit_time = pit_time;
  pit_ticks_last = pit_ticks;

  if(abort_test || (test_iterations == 0))
  {
      // disable interval timer
      itime.it_interval.tv_sec = 0;
      itime.it_interval.tv_nsec = 0;
      itime.it_value.tv_sec = 0;
      itime.it_value.tv_nsec = 0;
      timer_settime(timer_1, flags, &itime, &last_itime);
      abort_test=1;

      printf("Disabling interval timer with abort=%d\n", abort_test);
  }

}



int main(int argc, char *argv[])
{
   int rc, flags=0;
   unsigned int duration; // msecs
   struct timespec rtclk_resolution;
   long ticks_per_sec;
   double dt;
   struct tms systime;


   if(argc < 3) 
   {
       printf("usage: clock_test iterations msec\n");
       exit(-1);
   }
   else 
   {
       sscanf(argv[1], "%d", &test_iterations);
       sscanf(argv[2], "%u", &duration);
       duration_sec=duration/1000;
       duration_nsec=(duration % 1000)*1000000;
       duration_target_time = (double)duration_sec + ((double)duration_nsec/1000000000.0);

       printf("Requested %d iterations for %u msecs (%u sec, %u nsec) with target=%lf\n", test_iterations, duration, duration_sec, duration_nsec, duration_target_time);
   }

   printf("\n\nPIT based delay test for %u secs, %u nsecs\n", duration_sec, duration_nsec);

   if(clock_getres(CLOCK_REALTIME, &rtclk_resolution) == ERROR)
   {
      perror("clock_getres");
      exit(-1);
   }
   else
   {
      dt = (double)rtclk_resolution.tv_sec + ((double)rtclk_resolution.tv_nsec/1000000000.0);
      printf("\n\nClock resolution:\n\t%ld secs, %ld usecs, %ld nsecs OR %lf secs\n\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec, dt);

      ticks_per_sec = sysconf(_SC_CLK_TCK);

      printf("\nSystem ticks_per_sec=%ld\n\n", ticks_per_sec);
  }

   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();

   pthread_attr_init(&main_sched_attr);
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   main_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);


   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1);
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();

   // Get starting system tick time as a zero reference
   pit_tick_start=times(&systime);

   // REPLACE THREAD for DELAY-based clock thread with an interval timer signal
   //
   timer_create(CLOCK_REALTIME, NULL, &timer_1);

   signal(SIGALRM, (void(*)()) delay_test);


   /* arm the interval timer */
   itime.it_interval.tv_sec = duration_sec;
   itime.it_interval.tv_nsec = duration_nsec;
   itime.it_value.tv_sec = duration_sec;
   itime.it_value.tv_nsec = duration_nsec;

   // zero reference time for test
   clock_gettime(CLOCK_REALTIME, &last_pit_time);

   timer_settime(timer_1, flags, &itime, &last_itime);

   // main thread just busy waits for interval timer test iterations to complete
   while(!abort_test)
   {
       pause();
   }

   printf("Test aborted\n");

   if(pthread_attr_destroy(&main_sched_attr) != 0)
     perror("attr destroy");

   printf("TEST COMPLETE\n");

}


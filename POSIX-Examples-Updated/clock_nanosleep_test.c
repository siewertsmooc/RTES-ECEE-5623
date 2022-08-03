/*****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration              */
/*                                                                           */
/* Sam Siewert - 02/05/2011                                                  */
/*                                                                           */
/* Updated 7/28/2022 to demonstrate high resolution clock features and use   */
/*                   double precision float timestamps                       */
/*                                                                           */
/* Some useful references and man pages:                                     */
/*                                                                           */
/* man 7 time                                                                */
/* sudo cat /proc/time_list                                                  */
/* man 2 times                                                               */
/* sysconf(_SC_CLK_TCK)                                                      */
/* clock_getres                                                              */
/*                                                                           */
/* Note that sleep can wake up early                                         */
/* Note that high frequency calls to clock_gettime can cause timing glitches */
/* Note that a tick is usually 10 msec, and can be used for coarse timing    */
/*                                                                           */
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

#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)

static unsigned int sleep_count = 0;

pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;
static char syslogtag[80] = "MYTAG";

typedef struct
{
    int iterations;
    unsigned int duration;
} threadParams_t;


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




void *delay_test(void *threadp)
{
  int testidx;
  long ticks_per_sec;
  unsigned int duration_sec=1, duration_nsec=0;
  unsigned int max_sleep_calls=3;
  struct timespec rtclk_resolution;
  double dt, sleepdt;
  int numtests=10;
  struct timespec rtclk_start_time = {0, 0};
  struct timespec rtclk_stop_time = {0, 0};
  struct timespec sleep_time = {0, 0};
  struct timespec sleep_requested = {0, 0};
  struct timespec remaining_time = {0, 0};
  threadParams_t *tp = (threadParams_t *)threadp;

  struct tms systime;
  clock_t start_ticks, stop_ticks, dt_ticks;


  numtests = tp->iterations;
  duration_sec = (tp->duration)/1000;
  duration_nsec = ((tp->duration) % 1000)*1000000;

  printf("\n\nThread sleep based delay test for %u secs, %u nsecs\n", duration_sec, duration_nsec);

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

  for(testidx = 0; testidx < numtests; testidx++)
  {
      sleep_count = 0;

      /* run test for specified time in msecs */
      sleep_time.tv_sec=duration_sec;
      sleep_time.tv_nsec=duration_nsec;
      sleep_requested.tv_sec=sleep_time.tv_sec;
      sleep_requested.tv_nsec=sleep_time.tv_nsec;


      /* START: start time stamp */ 
      clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
      start_ticks=times(&systime);

      /* request sleep time and repeat if time remains */
      do 
      {
          nanosleep(&sleep_time, &remaining_time);
         
          sleep_time.tv_sec = remaining_time.tv_sec;
          sleep_time.tv_nsec = remaining_time.tv_nsec;
          sleep_count++;
      } 
      while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0)) && (sleep_count < max_sleep_calls));

      // STOP: stop time stamp 
      clock_gettime(CLOCK_REALTIME, &rtclk_stop_time);
      stop_ticks=times(&systime);

      dt=delta_t(&rtclk_stop_time, &rtclk_start_time);
      dt_ticks=stop_ticks - start_ticks;
      sleepdt = (double)sleep_requested.tv_sec + ((double)sleep_requested.tv_nsec/1000000000.0);

      printf("RT clock dt = %lf secs for sleep of %lf secs, err = %lf usec, sleepcnt=%d, ticks=%ld\n", dt, sleepdt, (dt-sleepdt)*1000000.0, sleep_count, dt_ticks);
      syslog(LOG_CRIT, "%s: RT clock dt = %lf secs for sleep of %lf secs, err = %lf usec, sleepcnt=%d, ticks=%ld\n", syslogtag, dt, sleepdt, (dt-sleepdt)*1000000.0, sleep_count, dt_ticks);

  }

  pthread_exit(0);
}



int main(int argc, char *argv[])
{
   int rc;

   threadParams_t tp;
   
   if(argc < 3) 
   {
       printf("usage: clock_test iterations msec [test TAG]\n");
       exit(-1);
   }
   else 
   {
       sscanf(argv[1], "%d", &tp.iterations);
       sscanf(argv[2], "%u", &tp.duration);
       if(argc == 4) sscanf(argv[3], "%s", syslogtag);
       printf("Requested %d iterations for %u msecs\n", tp.iterations, tp.duration);
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

   main_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&main_sched_attr, &main_param);


   // Pass in iterations and duration in msec as thread parameters
   rc = pthread_create(&main_thread, &main_sched_attr, delay_test, (void *)&tp);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror("pthread_create");
       exit(-1);
   }

   pthread_join(main_thread, NULL);

   if(pthread_attr_destroy(&main_sched_attr) != 0)
     perror("attr destroy");

   printf("TEST COMPLETE\n");

}


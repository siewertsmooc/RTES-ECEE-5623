#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREADS (4)
#define NUM_CPUS (4)

#define NSEC_PER_SEC (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_MICROSEC (1000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)


unsigned int idx = 0, jdx = 1;
unsigned int seqIterations = 47;
unsigned int reqIterations = 10000000;
volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;

int FIB_TEST(unsigned int seqCnt, unsigned int iterCnt)
{
   for(idx=0; idx < iterCnt; idx++)
   {
      fib = fib0 + fib1;
      while(jdx < seqCnt)
      { 
         fib0 = fib1;
         fib1 = fib;
         fib = fib0 + fib1;
         jdx++;
      }

      jdx=0;
   }

   return idx;
}


typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
pthread_attr_t main_attr;
pid_t mainpid;


int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  //printf("\ndt calcuation\n");

  // case 1 - less than a second of change
  if(dt_sec == 0)
  {
	  //printf("dt less than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = 0;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means stop is earlier than start
	  {
	         printf("stop is earlier than start\n");
		 return(ERROR);  
	  }
  }

  // case 2 - more than a second of change, check for roll-over
  else if(dt_sec > 0)
  {
	  //printf("dt more than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = dt_sec;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = delta_t->tv_sec + 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means roll over
	  {
	          //printf("nanosec roll over\n");
		  delta_t->tv_sec = dt_sec-1;
		  delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
	  }
  }

  return(OK);
}

#define SUM_ITERATIONS (1000)

void *counterThread(void *threadp)
{
    int sum=0, i, policy, rc;
    pthread_t thread;
    cpu_set_t cpuset;
    struct sched_param thread_schedp;
    struct timespec start_time = {0, 0};
    struct timespec finish_time = {0, 0};
    struct timespec thread_dt = {0, 0};
    threadParams_t *threadParams = (threadParams_t *)threadp;

    thread=pthread_self();
    CPU_ZERO(&cpuset);

    pthread_getschedparam(pthread_self(), &policy, &thread_schedp);

    printf("thread prio=%d for %d\n", thread_schedp.sched_priority, pthread_self());

    // Test elevation of priority
    if(rc=pthread_setschedprio(pthread_self(), thread_schedp.sched_priority+1) < 0)
	    perror("pthread_setschedprio");
    else
    {
            pthread_getschedparam(pthread_self(), &policy, &thread_schedp);
	    printf("NEW thread prio=%d for %d\n", thread_schedp.sched_priority, pthread_self());
    }


    clock_gettime(CLOCK_REALTIME, &start_time);
    // COMPUTE SECTION
    for(i=1; i < ((threadParams->threadIdx)+1*SUM_ITERATIONS); i++)
        sum=sum+i;

    FIB_TEST(seqIterations, reqIterations);
    // END COMPUTE SECTION
    clock_gettime(CLOCK_REALTIME, &finish_time);



    //printf("\nThread idx=%d, sum[0...%d]=%d\n", 
    //       threadParams->threadIdx,
    //       (threadParams->threadIdx+1)*SUM_ITERATIONS, sum);

    delta_t(&finish_time, &start_time, &thread_dt);

    printf("\nThread idx=%d ran %ld sec, %ld msec (%ld microsec, %ld nsec) on core=%d\n",
	   threadParams->threadIdx, thread_dt.tv_sec, (thread_dt.tv_nsec / NSEC_PER_MSEC),
	   (thread_dt.tv_nsec / NSEC_PER_MICROSEC), thread_dt.tv_nsec, sched_getcpu());

    pthread_exit(&sum);
}


void print_scheduler(void)
{
   int schedType, scope;

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

   pthread_attr_getscope(&main_attr, &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)
     printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
     printf("PTHREAD SCOPE PROCESS\n");
   else
     printf("PTHREAD SCOPE UNKNOWN\n");

}



int main (int argc, char *argv[])
{
   int rc, idx;

   printf("This system has %d processors with %d available\n", get_nprocs_conf(), get_nprocs());
   printf("The test thread created will be SCHED_FIFO is run with sudo and will be run on least busy core\n");

   mainpid=getpid();

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   print_scheduler();
   rc=sched_getparam(mainpid, &main_param);
   main_param.sched_priority=rt_max_prio;

   if(rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param) < 0)
   {
	   perror("******** WARNING: sched_setscheduler");
	   printf("MUST RUN SCHED_FIFO as SUDO ROOT\n");
	   exit(-1);
   }

   print_scheduler();


   printf("rt_max_prio=%d\n", rt_max_prio);
   printf("rt_min_prio=%d\n", rt_min_prio);


   for(idx=0; idx < NUM_THREADS; idx++)
   {
       rc=pthread_attr_init(&rt_sched_attr[idx]);
       rc=pthread_attr_setinheritsched(&rt_sched_attr[idx], PTHREAD_EXPLICIT_SCHED);
       rc=pthread_attr_setschedpolicy(&rt_sched_attr[idx], SCHED_FIFO);

       rt_param[idx].sched_priority=rt_max_prio-idx-1;
       pthread_attr_setschedparam(&rt_sched_attr[idx], &rt_param[idx]);

       threadParams[idx].threadIdx=idx;

       pthread_create(&threads[idx],               // pointer to thread descriptor
                      &rt_sched_attr[idx],         // use SPECIFIC SECHED_FIFO attributes
                      counterThread,               // thread function entry point
                      (void *)&(threadParams[idx]) // parameters to pass in
                     );

   }

   for(idx=0;idx<NUM_THREADS;idx++)
       pthread_join(threads[idx], NULL);

   printf("\nTEST COMPLETE\n");
}

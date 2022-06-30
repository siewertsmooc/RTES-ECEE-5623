/*
Code adapted from Dr. Sam Siewarts Example Code
https://github.com/siewertsmooc/RTES-ECEE-5623
*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define COUNT  100
#define NUM_THREADS 3

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

#define SCHED_POLICY SCHED_FIFO
#define MAX_ITERATIONS (1)

// Thread specific globals
int gsum[NUM_THREADS]={0, 100, 200};

void print_scheduler(void)
{
    int schedType = sched_getscheduler(getpid());

    switch(schedType)
    {
        case SCHED_FIFO:
            printf("Pthread policy is SCHED_FIFO\n");
            break;
        case SCHED_OTHER:
            printf("Pthread policy is SCHED_OTHER\n");
            break;
        case SCHED_RR:
            printf("Pthread policy is SCHED_RR\n");
            break;
        default:
            printf("Pthread policy is UNKNOWN\n");
    }
}

void *sumThread(void *threadp)
{
    int i, idx;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    idx = threadParams->threadIdx;

    for(i=1; i<COUNT; i++)
    {
        gsum[idx] = gsum[idx] + (i + (idx*COUNT));
        printf("Increment %d for thread idx=%d, gsum=%d\n", i, idx, gsum[idx]);
    }
}

int main (int argc, char *argv[])
{
   int rc, idx;
   int testsum = 0;

   mainpid=getpid();

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   rc=sched_getparam(mainpid, &main_param);
   main_param.sched_priority=rt_max_prio;

   if(rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param) < 0)
   {
	   perror("******** WARNING: sched_setscheduler");
	   printf("MUST RUN SCHED_FIFO as SUDO ROOT\n");
	   exit(-1);
   }
   print_scheduler();

   for(idx=0; idx < NUM_THREADS; idx++)
   {
       rc=pthread_attr_init(&rt_sched_attr[idx]);
       rc=pthread_attr_setinheritsched(&rt_sched_attr[idx], PTHREAD_EXPLICIT_SCHED);
       rc=pthread_attr_setschedpolicy(&rt_sched_attr[idx], SCHED_FIFO);

       // Adjust the priority based on which thread is running
       rt_param[idx].sched_priority=rt_max_prio-idx-1;
       pthread_attr_setschedparam(&rt_sched_attr[idx], &rt_param[idx]);

       threadParams[idx].threadIdx=idx;

       pthread_create(&threads[idx],               // pointer to thread descriptor
                      &rt_sched_attr[idx],         // use SPECIFIC SECHED_FIFO attributes
                      sumThread,                   // thread function entry point
                      (void *)&(threadParams[idx]) // parameters to pass in
                     );

   }

   for(idx=0;idx<NUM_THREADS;idx++)
       pthread_join(threads[idx], NULL);

/* Original Source Code
   for(i=0; i<3; i++)
   {
      threadParams[i].threadIdx=i;
      pthread_create(&threads[i], (void *)0, sumThread, (void *)&(threadParams[i]));
   }

   for(i=0; i<3; i++)
     pthread_join(threads[i], NULL);
*/

   printf("TEST COMPLETE: gsum[0]=%d, gsum[1]=%d, gsum[2]=%d, gsumall=%d\n", 
          gsum[0], gsum[1], gsum[2], (gsum[0]+gsum[1]+gsum[2]));

   // Verfiy with single thread version and (n*(n+1))/2
   for(int i=1; i<(3*COUNT); i++)
       testsum = testsum + i;

   printf("TEST COMPLETE: testsum=%d, [n[n+1]]/2=%d\n", testsum, ((3*COUNT-1)*(3*COUNT))/2);
}

    
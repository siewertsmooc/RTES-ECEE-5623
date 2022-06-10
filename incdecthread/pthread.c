#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>

#define COUNT  1000
#define SCHED_POLICY SCHED_FIFO

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];
pthread_attr_t rt_sched_attr;
struct sched_param rt_param;
sem_t startIOWorker[2];

int rt_max_prio, rt_min_prio;

// Unsafe global
int gsum=0;

void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    sem_wait (&(startIOWorker[threadParams->threadIdx]));

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum+i;
        printf("Increment thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }

    sem_post (&(startIOWorker[i]));
}


void *decThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    sem_wait (&(startIOWorker[threadParams->threadIdx]));

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum-i;
        printf("Decrement thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }

    sem_post (&(startIOWorker[i]));
}

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

void setSchedPolicy(void)
{
   int rc, scope;

   // Set up scheduling
   print_scheduler();

   pthread_attr_init(&rt_sched_attr);
   pthread_attr_setinheritsched(&rt_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_POLICY);

   rt_max_prio = sched_get_priority_max(SCHED_POLICY);
   rt_min_prio = sched_get_priority_min(SCHED_POLICY);

   rt_param.sched_priority = rt_max_prio;

   rc=sched_setscheduler(getpid(), SCHED_POLICY, &rt_param);

   if (rc)
   {
       printf("ERROR: sched_setscheduler rc is %d\n", rc);
       perror("sched_setscheduler");
   }
   else
   {
       printf("SCHED_POLICY SET: sched_setscheduler rc is %d\n", rc);
   }

   print_scheduler();
}


int main (int argc, char *argv[])
{
   int rc;
   int i=0;

   setSchedPolicy();

   threadParams[i].threadIdx=i;

   sem_init (&(startIOWorker[i]), 0, 0);
   pthread_create(&threads[i],   // pointer to thread descriptor
                  &rt_sched_attr,     // use default attributes
                  incThread, // thread function entry point
                  (void *)&(threadParams[i]) // parameters to pass in
                 );
    sem_destroy (&(startIOWorker[i]));


   i++;
   threadParams[i].threadIdx=i;
   sem_init (&(startIOWorker[i]), 0, 0);
   pthread_create(&threads[i], &rt_sched_attr, decThread, (void *)&(threadParams[i]));

    sem_destroy (&(startIOWorker[i]));


   for(i=0; i<2; i++)
     pthread_join(threads[i], NULL);

   printf("TEST COMPLETE\n");
}

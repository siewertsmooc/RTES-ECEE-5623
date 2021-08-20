#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

// This example creates a NUM_THREADS which all compute the 
// Fibonacci sequence concurrently.  It is useful to learn
// about the POSIX API to OS threads and to determine if 
// threading will speed up computations.

#include "perflib.h"
#include "fib.h"

#define NUM_THREADS 32

pthread_t threads[NUM_THREADS];
double DT[NUM_THREADS];
double startTOD;
double stopTOD;

// POSIX thread declarations and scheduling attributes
//
pthread_t main_thread;
pthread_attr_t rt_sched_attr;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min, old_ceiling, new_ceiling;
struct sched_param rt_param;
struct sched_param nrt_param;
struct sched_param main_param;


// Fibonacci workload to compute for each thread created.
// Note that this sequence computation was selected because
// of the data dependencies that prevent compilers from
// over optimizing workload code - the compiler can
// optimize, but can't eliminate code entirely for computation
// of this sequence.
//

UINT64 reqIterations = 1;
UINT64 seqIterations = FIB_LIMIT_FOR_32_BIT, Iterations = 1;

void *fibSeq(void *threadid)
{
   UINT64 fib = 0, fib0 = 0, fib1 = 1;
   UINT32 idx = 0, jdx = 1;

   // START Timed Fibonacci Test
   FIB_TEST(seqIterations, Iterations);

   //printf("\nFibonacci(%llu)=%llu\n", seqIterations, fib);
   //printf("fibSeq %d stopping\n", threadid);
   //pthread_exit(threadid);
}


// Simply prints the current POSIX scheduling policy in effect.
//
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


void *testThread(void *threadid)
{
   double DT=0.0;
   int i;

   // THREADED TEST
   //

   // Start time for all threads
   startTOD=readTOD();

   for(i=0;i<NUM_THREADS;i++)
       pthread_create(&threads[i], &rt_sched_attr, fibSeq, (void *)i);

   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   // Stop time for all threads
   stopTOD=readTOD();

   DT = elapsedTOD(stopTOD, startTOD);
   printf("Threaded:   Fib(%llu)\t\t\t\t%lf secs\n", (seqIterations*Iterations), DT);


   // SEQUENTIAL TEST
   //

   // Start time for sequential function calls
   startTOD=readTOD();

   for(i=0;i<NUM_THREADS;i++)
       fibSeq((void *)i);

   // Stop time for sequential function calls
   stopTOD=readTOD();

   DT = elapsedTOD(stopTOD, startTOD);
   printf("Sequential: Fib(%llu)\t\t\t\t%lf secs\n", (seqIterations*Iterations), DT);

}


int main (int argc, char *argv[])
{
   int rc, scope;

   if(argc == 2)
   {
      sscanf(argv[1], "%llu", &reqIterations);

      seqIterations = reqIterations % FIB_LIMIT_FOR_32_BIT;
      Iterations = reqIterations / seqIterations;
      //printf("request=%llu, seqIter=%llu, iter=%llu, total=%llu, Diff=%lld\n", reqIterations, seqIterations, Iterations, (seqIterations*Iterations), (reqIterations - (seqIterations*Iterations))); 
   }
   else if(argc == 1)
      printf("Using defaults\n");
   else
      printf("Usage: fibtest [Num iterations]\n");

   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();

   // Set POSIX Scheduling Policy
   //
   // Note that FIFO is essentially priority preemptive run to
   // completion on Linux with NPTL since each thread will run
   // uninterrupted at it's given priority level.
   //
   // RR allows threads to run in a Round Robin fashion.
   //
   // We set all threads to be run to completion at high 
   // priority so that we can determine whether the hardware
   // provides speed-up by mapping threads onto multiple cores
   // and/or SMT (Symmetric Multi-threading) on each core.
   //
   pthread_attr_init(&rt_sched_attr);
   pthread_attr_init(&main_sched_attr);
   pthread_attr_setinheritsched(&rt_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_FIFO);
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   rc=sched_getparam(getpid(), &nrt_param);
   rt_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &rt_param);

   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc); perror(NULL); exit(-1);
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();

   printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);
   pthread_attr_getscope(&rt_sched_attr, &scope);

   // Check the scope of the POSIX scheduling mechanism
   //
   if(scope == PTHREAD_SCOPE_SYSTEM)
     printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
     printf("PTHREAD SCOPE PROCESS\n");
   else
     printf("PTHREAD SCOPE UNKNOWN\n");

   // Note that POSIX priorities are such that the highest priority
   // thread has a large priority number.  This is very different
   // than VxWorks for example where low priority numbers mean high
   // scheduling priority.  As long as the sched_get_priority_max(<policy>)
   // function call is used, then the number is not important.
   //
   // IMPORTANT: for this test, note that the thread that creates all other
   //            threads has higher priority than the workload threads
   //            themselves - this prevents it from being preempted so that
   //            it can continue to create all threads in order to get them
   //            concurrently active.
   //
   rt_param.sched_priority = rt_max_prio-1;
   pthread_attr_setschedparam(&rt_sched_attr, &rt_param);

   main_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&main_sched_attr, &main_param);

   rc = pthread_create(&main_thread, &main_sched_attr, testThread, (void *)0);
   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);
   }

   pthread_join(main_thread, NULL);

   if(pthread_attr_destroy(&rt_sched_attr) != 0)
     perror("attr destroy");

   rc=sched_setscheduler(getpid(), SCHED_OTHER, &nrt_param);

   printf("TEST COMPLETE\n");

}

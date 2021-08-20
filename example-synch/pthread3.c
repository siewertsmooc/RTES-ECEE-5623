//  For this demonstation to work on multi-core systems, all threads must run on one shared core as
//  would be the case for an AMP multi-service system with more than one service running on each core.
//
//  This can be done by setting CPU core affinity in the code.
//
//  This can also be done with "sudo taskset -c 0 ./pthread3" on the command line.
//
//  SMP Linux features can and will assign different cores to threads and can also migrate them if affinity is
//  not set.
//

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS		4
#define START_SERVICE 		0
#define HIGH_PRIO_SERVICE 	1
#define MID_PRIO_SERVICE 	2
#define LOW_PRIO_SERVICE 	3
#define CS_LENGTH 		10

pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr;
pthread_attr_t nrt_sched_attr;
int rt_max_prio, rt_min_prio;
struct sched_param rt_param;
struct sched_param nrt_param;
struct sched_param main_param;

typedef struct
{
    int threadIdx;
} threadParams_t;


threadParams_t threadParams[NUM_THREADS];

pthread_mutex_t sharedMemSem;
pthread_mutexattr_t rt_safe;

int rt_protocol;

volatile int runInterference=0, CScnt=0;
volatile unsigned idleCount[NUM_THREADS];
int intfTime=0;

int numberOfProcessors;
struct timespec timeNow, timeStartTest;

unsigned const int fibLength = 47;  // if number is too large, unsigned will overflow
unsigned const int fibComputeSequences = 100000;   // to add more time, increase iterations

// Helper functions
void fibCycleBurner(unsigned seqCnt, unsigned iterCnt, int traceOn);
void *startService(void *threadid);
double dTime(struct timespec now, struct timespec start);
void print_scheduler(void);

// function entry points for 2 tasks used in this demonstration
void *simpleTask(void *threadp);
void *criticalSectionTask(void *threadp);



int main (int argc, char *argv[])
{
   int rc, invSafe=0, i, scope;
   struct timespec sleepTime;
   cpu_set_t threadcpu;
   int coreid;

   printf("Fibonacci Cycle Burner test ...\n");
   fibCycleBurner(47, 2, 1);
   printf("\ndone\n");

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   if(argc < 2)
   {
     printf("Usage: pthread interfere-seconds\n");
     exit(-1);
   }
   else if(argc >= 2)
   {
     sscanf(argv[1], "%d", &intfTime);
     printf("interference time = %d secs\n", intfTime);
     printf("unsafe mutex will be created\n");
   }

   print_scheduler();
   rc=sched_getparam(getpid(), &nrt_param);


   CPU_ZERO(&threadcpu);
   coreid=0;
   printf("Setting thread %d to core %d\n", i, coreid);
   CPU_SET(coreid, &threadcpu);
   for(i=0; i<numberOfProcessors; i++)
       if(CPU_ISSET(i, &threadcpu))  printf(" CPU-%d ", i);
   printf("\nLaunching thread %d\n", i);

   pthread_attr_init(&rt_sched_attr);
   pthread_attr_setinheritsched(&rt_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_FIFO);
   pthread_attr_setaffinity_np(&rt_sched_attr, sizeof(cpu_set_t), &threadcpu);

   pthread_attr_init(&nrt_sched_attr);
   pthread_attr_setinheritsched(&rt_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_RR);
   //pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_OTHER);


   if (rc) 
   {
       printf("ERROR - run with sudo; sched_setscheduler rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);
   pthread_attr_getscope(&rt_sched_attr, &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)
     printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
     printf("PTHREAD SCOPE PROCESS\n");
   else
     printf("PTHREAD SCOPE UNKNOWN\n");

   pthread_mutex_init(&sharedMemSem, NULL);

   // Set priority lower than H and M, but just a bit higher than L
   rt_param.sched_priority = rt_min_prio+1;
   pthread_attr_setschedparam(&rt_sched_attr, &rt_param);

   printf("\nCreating RT thread %d\n", START_SERVICE);
   threadParams[START_SERVICE].threadIdx=START_SERVICE;
   rc = pthread_create(&threads[START_SERVICE], &rt_sched_attr, startService, (void *)&threadParams[START_SERVICE]);

   if (rc)
   {
       printf("ERROR - run with sudo; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   printf("Start services thread spawned\n");


   printf("will join service threads\n");

   if(pthread_join(threads[START_SERVICE], NULL) == 0)
     printf("START SERVICE joined\n");
   else
     perror("START SERVICE");


   if(pthread_mutex_destroy(&sharedMemSem) != 0)
     perror("mutex destroy");

   printf("All threads done\n");

   exit(0);
}



void *startService(void *threadid)
{
   int rc, busyWaitCnt=0;

   runInterference=intfTime;
   gettimeofday(&timeStartTest, (void *)0);



   // CREATE L Thread as Non-RT BE or lowest prio RT thread and make sure it enters the C.S. before starting H
   //
   rt_param.sched_priority = rt_min_prio;
   pthread_attr_setschedparam(&rt_sched_attr, &rt_param);

   printf("\nCreating Low Prio RT or BE thread %d\n", LOW_PRIO_SERVICE);
   threadParams[LOW_PRIO_SERVICE].threadIdx=LOW_PRIO_SERVICE;

   // Real-time option
   //rc = pthread_create(&threads[LOW_PRIO_SERVICE], &rt_sched_attr, criticalSectionTask, (void *)&threadParams[LOW_PRIO_SERVICE]);

   // Non-real-time option
   rc = pthread_create(&threads[LOW_PRIO_SERVICE], &nrt_sched_attr, criticalSectionTask, (void *)&threadParams[LOW_PRIO_SERVICE]);

   if (rc)
   {
       printf("ERROR - run with sudo; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[LOW_PRIO_SERVICE]);
   gettimeofday(&timeNow, (void *)0);
   printf("Low prio %d thread SPAWNED at %lf sec\n", LOW_PRIO_SERVICE, dTime(timeNow, timeStartTest));


   // spin until L enters the critical section
   while(CScnt < 1)
   {
       busyWaitCnt++; 
       if((busyWaitCnt % 10000) == 0) {printf(".");};
       //if((busyWaitCnt % 10) == 0) {sleep(1); printf(".");};
   }
   printf("CScnt=%d\n", CScnt);



   // CREATE H Thread as RT thread at highest priority, but it will block on C.S. semaphore held by L until
   // L finishes the C.S.
   //
   rt_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&rt_sched_attr, &rt_param);


   printf("\nCreating RT thread %d, CScnt=%d\n", HIGH_PRIO_SERVICE, CScnt);
   threadParams[HIGH_PRIO_SERVICE].threadIdx=HIGH_PRIO_SERVICE;
   //rc = pthread_create(&threads[HIGH_PRIO_SERVICE], &rt_sched_attr, simpleTask, (void *)&threadParams[HIGH_PRIO_SERVICE]);
   rc = pthread_create(&threads[HIGH_PRIO_SERVICE], &rt_sched_attr, criticalSectionTask, (void *)&threadParams[HIGH_PRIO_SERVICE]);

   if (rc)
   {
       printf("ERROR - run with sudo; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[HIGH_PRIO_SERVICE]);
   gettimeofday(&timeNow, (void *)0);
   printf("High prio %d thread SPAWNED at %lf sec\n", HIGH_PRIO_SERVICE, dTime(timeNow, timeStartTest));



   // CREATE M Thread as RT thread at any lower priority than H, but higher than L so that L is interfered with and
   // cannot complete the C.S. until M is done with any amount of computation (unbounded).
   //
   if(runInterference > 0)
   {
       rt_param.sched_priority = rt_max_prio-1;
       pthread_attr_setschedparam(&rt_sched_attr, &rt_param);

       printf("\nCreating RT thread %d\n", MID_PRIO_SERVICE);
       threadParams[MID_PRIO_SERVICE].threadIdx=MID_PRIO_SERVICE;
       rc = pthread_create(&threads[MID_PRIO_SERVICE], &rt_sched_attr, simpleTask, (void *)&threadParams[MID_PRIO_SERVICE]);

       if (rc)
       {
           printf("ERROR - run with sudo; pthread_create() rc is %d\n", rc);
           perror(NULL);
           exit(-1);
       }
       //pthread_detach(threads[MID_PRIO_SERVICE]);
       gettimeofday(&timeNow, (void *)0);
       printf("Middle prio %d thread SPAWNED at %lf sec\n", MID_PRIO_SERVICE, dTime(timeNow, timeStartTest));
    }



   if(pthread_join(threads[HIGH_PRIO_SERVICE], NULL) == 0)
     printf("HIGH PRIO joined\n");
   else
     perror("HIGH PRIO");


   if(runInterference > 0)
   {
       if(pthread_join(threads[MID_PRIO_SERVICE], NULL) == 0)
         printf("MID PRIO joined\n");
       else
         perror("MID PRIO");
   }


   if(pthread_join(threads[LOW_PRIO_SERVICE], NULL) == 0)
     printf("LOW PRIO joined\n");
   else
     perror("LOW PRIO");


   pthread_exit(NULL);

}


double dTime(struct timespec now, struct timespec start)
{
    double nowReal=0.0, startReal=0.0;

    nowReal = (double)now.tv_sec + ((double)now.tv_nsec / 1000000000.0);
    startReal = (double)start.tv_sec + ((double)start.tv_nsec / 1000000000.0);

    return (nowReal-startReal);
}

void fibCycleBurner(unsigned seqCnt, unsigned iterCnt, int traceOn)
{
   volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;
   int idx, jdx=1;

   if(traceOn) printf("%u %u ", fib0, fib1);

   for(idx=0; idx < iterCnt; idx++)    
   {                                   
      fib = fib0 + fib1;               
      if(traceOn) printf("%u ", fib);

      while(jdx < seqCnt)              
      {                                
         fib0 = fib1;                 
         fib1 = fib;                  
         fib = fib0 + fib1;            
         if(traceOn) printf("%u ", fib);
         jdx++;                        
      }                                
      jdx=1; 
      fib = 0, fib0 = 0, fib1 = 1;                        
      if(traceOn && (idx < iterCnt-1)) printf("\n\n%u %u ", fib0, fib1);
   }                                   
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
	   printf("Pthread Policy is SCHED_RR\n");
	   break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}


void *simpleTask(void *threadp)
{
  struct timespec timeNow;
  pthread_t thread;
  cpu_set_t cpuset;
  threadParams_t *threadParams = (threadParams_t *)threadp;
  int idleIdx = threadParams->threadIdx, cpucore;

  thread=pthread_self();
  cpucore=sched_getcpu();

  CPU_ZERO(&cpuset);
  pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

  do
  {
    fibCycleBurner(fibLength, fibComputeSequences, 0);
    idleCount[idleIdx]++;
    if(idleIdx == LOW_PRIO_SERVICE) printf("L%u ", idleCount[idleIdx]);
    else if(idleIdx == MID_PRIO_SERVICE) printf("M%u ", idleCount[idleIdx]);
    else if(idleIdx == HIGH_PRIO_SERVICE) printf("H%u ", idleCount[idleIdx]);
  } while(idleCount[idleIdx] < runInterference);

  gettimeofday(&timeNow, (void *)0);

  if(idleIdx == LOW_PRIO_SERVICE)
      printf("\n**** LOW PRIO %d on core %d INTERFERE NO SEM COMPLETED at %lf sec\n", idleIdx, cpucore, dTime(timeNow, timeStartTest));
  else if(idleIdx == MID_PRIO_SERVICE)
      printf("\n**** MID PRIO %d on core %d INTERFERE NO SEM COMPLETED at %lf sec\n", idleIdx, cpucore, dTime(timeNow, timeStartTest));
  else if(idleIdx == HIGH_PRIO_SERVICE)
      printf("\n**** HIGH PRIO %d on core %d INTERFERE NO SEM COMPLETED at %lf sec\n", idleIdx, cpucore, dTime(timeNow, timeStartTest));

  pthread_exit(NULL);

}


void *criticalSectionTask(void *threadp)
{
  struct timespec timeNow;
  pthread_t thread;
  threadParams_t *threadParams = (threadParams_t *)threadp;
  int idleIdx = threadParams->threadIdx, cpucore;

  thread=pthread_self();
  cpucore=sched_getcpu();

  if(idleIdx == LOW_PRIO_SERVICE) printf("\nCS-L REQUEST\n");
  else if(idleIdx == MID_PRIO_SERVICE) printf("\nCS-M REQUEST\n");
  else if(idleIdx == HIGH_PRIO_SERVICE) printf("\nCS-H REQUEST\n");

  pthread_mutex_lock(&sharedMemSem);
  CScnt++;

  if(idleIdx == LOW_PRIO_SERVICE) printf("\nCS-L ENTRY %u\n", CScnt);
  else if(idleIdx == MID_PRIO_SERVICE) printf("\nCS-M ENTRY %u\n", CScnt);
  else if(idleIdx == HIGH_PRIO_SERVICE) printf("\nCS-H ENTRY %u\n", CScnt);

  idleCount[idleIdx]=0;

  do
  {
    fibCycleBurner(fibLength, fibComputeSequences, 0);
    idleCount[idleIdx]++;
    if(idleIdx == LOW_PRIO_SERVICE) printf("CS-L%u ", idleCount[idleIdx]);
    else if(idleIdx == MID_PRIO_SERVICE) printf("CS-M%u ", idleCount[idleIdx]);
    else if(idleIdx == HIGH_PRIO_SERVICE) printf("CS-H%u ", idleCount[idleIdx]);
  } while(idleCount[idleIdx] < CS_LENGTH);

  if(idleIdx == LOW_PRIO_SERVICE) printf("\nCS-L LEAVING\n");
  else if(idleIdx == MID_PRIO_SERVICE) printf("\nCS-M LEAVING\n");
  else if(idleIdx == HIGH_PRIO_SERVICE) printf("\nCS-H LEAVING\n");

  pthread_mutex_unlock(&sharedMemSem);

  if(idleIdx == LOW_PRIO_SERVICE) printf("\nCS-L EXIT\n");
  else if(idleIdx == MID_PRIO_SERVICE) printf("\nCS-M EXIT\n");
  else if(idleIdx == HIGH_PRIO_SERVICE) printf("\nCS-H EXIT\n");

  gettimeofday(&timeNow, (void *)0);

  if(idleIdx == LOW_PRIO_SERVICE)
      printf("\n**** LOW PRIO %d on core %d CRIT SECTION WORK COMPLETED at %lf sec\n", idleIdx, cpucore, dTime(timeNow, timeStartTest));
  else if(idleIdx == MID_PRIO_SERVICE)
      printf("\n**** MID PRIO %d on core %d CRIT SECTION WORK COMPLETED at %lf sec\n", idleIdx, cpucore, dTime(timeNow, timeStartTest));
  else if(idleIdx == HIGH_PRIO_SERVICE)
      printf("\n**** HIGH PRIO %d on core %d CRIT SECTION WORK COMPLETED at %lf sec\n", idleIdx, cpucore, dTime(timeNow, timeStartTest));

  pthread_exit(NULL);

}


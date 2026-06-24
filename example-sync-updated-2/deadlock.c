/*
 * Task: Fix the deadlock so that it does not occur by using a random back-off scheme to resolve.
 * Author: Mason McGaffin
 */
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_THREADS 2
#define THREAD_0 0
#define THREAD_1 1

typedef struct
{
    int threadIdx;
} threadParams_t;


pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

struct sched_param nrt_param;

// On the Raspberry Pi, the MUTEX semaphores must be statically initialized
//
// This works on all Linux platforms, but dynamic initialization does not work
// on the R-Pi in particular as of June 2020.
//
pthread_mutex_t rsrcA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rsrcB = PTHREAD_MUTEX_INITIALIZER;

volatile int rsrcACnt=0, rsrcBCnt=0, noWait=0;


void *grabRsrcs(void *threadp)
{
  threadParams_t *threadParams = (threadParams_t *)threadp;
  int threadIdx = threadParams->threadIdx;

  while(1) {
    if(threadIdx == THREAD_0)
    {
      //keep original resource grab the same
      printf("Thread 0 grabbing resources\n");
      pthread_mutex_lock(&rsrcA);
      rsrcACnt++;
      if(!noWait) sleep(1);
      printf("Thread 0 got A, trying for B\n");
      
      // add random backoff try
      if(0 == pthread_mutex_trylock(&rsrcB))
      {
        rsrcBCnt++;
        printf("Thread 0 got A and B\n");
        pthread_mutex_unlock(&rsrcB);
        pthread_mutex_unlock(&rsrcA);
        printf("Thread 0 done\n");

        break;
      }
      else
      {
        //random backoff when lock fails
        printf("Thread 0 failed to get B. Backing off...\n");
        pthread_mutex_unlock(&rsrcA);

        usleep(rand() % 10000 + 1000); // sleep for 1-10ms
      }
    }
    else 
    {
      //keep original resource grab the same
      printf("Thread 1 grabbing resources\n");
      pthread_mutex_lock(&rsrcB);
      rsrcBCnt++;
      if(!noWait) sleep(1);
      printf("Thread 1 got B, trying for A\n");
      
      // add random backoff
      if(0 == pthread_mutex_trylock(&rsrcA))
      {
        rsrcACnt++;
        printf("Thread 1 got B and A\n");
        pthread_mutex_unlock(&rsrcA);
        pthread_mutex_unlock(&rsrcB);
        printf("Thread 1 done\n");
        break;
      }
      else
      {
        //backoff
        printf("Thread 1 failed to get A. Backing off...\n");
        pthread_mutex_unlock(&rsrcB);

        usleep(rand() % 10000 + 1000); // sleep for 1-10ms
      }
    }
  }
  pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
  int rc, safe=0;

  rsrcACnt=0, rsrcBCnt=0, noWait=0;

  //seed random num generator
  srand(time(NULL));

  if(argc < 2)
  {
    printf("Will set up random backoff scenario\n");
  }
  else if(argc == 2)
  {
    if(strncmp("safe", argv[1], 4) == 0)
      safe=1;
    else if(strncmp("race", argv[1], 4) == 0)
      noWait=1;
    else
      printf("Will set up random backoff scenario\n");
  }
  else
  {
    printf("Usage: deadlock [safe|race|unsafe]\n");
  }


  printf("Creating thread %d\n", THREAD_0);
  threadParams[THREAD_0].threadIdx=THREAD_0;
  rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)&threadParams[THREAD_0]);
  if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
  printf("Thread 0 spawned\n");

  if(safe) // Make sure Thread 0 finishes with both resources first
  {
    if(pthread_join(threads[0], NULL) == 0)
      printf("Thread 0: %x done\n", (unsigned int)threads[0]);
    else
      perror("Thread 0");
  }

  printf("Creating thread %d\n", THREAD_1);
  threadParams[THREAD_1].threadIdx=THREAD_1;
  rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)&threadParams[THREAD_1]);
  if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
  printf("Thread 1 spawned\n");

  printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
  printf("will try to join CS threads unless they deadlock\n");

  if(!safe)
  {
    if(pthread_join(threads[0], NULL) == 0)
      printf("Thread 0: %x done\n", (unsigned int)threads[0]);
    else
      perror("Thread 0");
  }

  if(pthread_join(threads[1], NULL) == 0)
    printf("Thread 1: %x done\n", (unsigned int)threads[1]);
  else
    perror("Thread 1");

  if(pthread_mutex_destroy(&rsrcA) != 0)
    perror("mutex A destroy");

  if(pthread_mutex_destroy(&rsrcB) != 0)
    perror("mutex B destroy");

  printf("All done\n");

  exit(0);
}

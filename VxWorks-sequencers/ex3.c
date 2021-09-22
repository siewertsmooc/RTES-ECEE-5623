/****************************************************************************/
/*                                                                          */
/* Sam Siewert - 2005                                                       */ 
/*		                                                                    */
/* Example #3                                                               */
/*
/* LCM = 15
/*
/* S1: T1=3,  C1=1, U1=0.3333
/* S2: T2=5,  C2=2, U2=0.4 
/* S3: T3=15, C3=3, U3=0.2
/*
/* Utotal = 0.93333, RM LUB = 3x(2^1/3-1)=0.78
/*
/* All times are milliseconds
/*
/****************************************************************************/

#include "vxWorks.h"
#include "semLib.h"
#include "sysLib.h"
#include "wvLib.h"

#define FIB_LIMIT_FOR_32_BIT 47

SEM_ID semS1, semS2, semS3; 
int abortTest = 0;
UINT32 seqIterations = FIB_LIMIT_FOR_32_BIT;
UINT32 fib1Cnt=0, fib2Cnt=0, fib3Cnt=0;
char ciMarker[]="CI";


#define FIB_TEST(seqCnt, iterCnt, idx, jdx, fib, fib0, fib1)      \
   for(idx=0; idx < iterCnt; idx++)                               \
   {                                                              \
      fib = fib0 + fib1;                                          \
      while(jdx < seqCnt)                                         \
      {                                                           \
         fib0 = fib1;                                             \
         fib1 = fib;                                              \
         fib = fib0 + fib1;                                       \
         jdx++;                                                   \
      }                                                           \
   }                                                              \


/* Iterations, 2nd arg must be tuned for any given target type
   using windview
   
   170000 <= 10 msecs on 100 MhZ Pentium - adjust as needed for C's
   
   Be very careful of WCET overloading CPU during first period of
   LCM.
   
 */
void fibS1(void)
{
UINT32 idx = 0, jdx = 1;
UINT32 fib = 0, fib0 = 0, fib1 = 1;

   while(!abortTest)
   {
	   semTake(semS1, WAIT_FOREVER);
	   FIB_TEST(seqIterations, 17500, idx, jdx, fib, fib0, fib1);
	   fib1Cnt++;
   }
}

void fibS2(void)
{
UINT32 idx = 0, jdx = 1;
UINT32 fib = 0, fib0 = 0, fib1 = 1;

   while(!abortTest)
   {
	   semTake(semS2, WAIT_FOREVER);
	   FIB_TEST(seqIterations, 35000, idx, jdx, fib, fib0, fib1);
	   fib2Cnt++;
   }
}

void fibS3(void)
{
UINT32 idx = 0, jdx = 1;
UINT32 fib = 0, fib0 = 0, fib1 = 1;

   while(!abortTest)
   {
	   semTake(semS3, WAIT_FOREVER);
	   FIB_TEST(seqIterations, 52500, idx, jdx, fib, fib0, fib1);
	   fib3Cnt++;
   }
}

void shutdown(void)
{
	abortTest=1;
}

void Sequencer(void)
{

  printf("Starting Sequencer\n");

  /* Just to be sure we have 1 msec tick and TOs */
  sysClkRateSet(1000);

  /* Set up service release semaphores */
  semS1 = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
  semS2 = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
  semS3 = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
 
  if(taskSpawn("service-S1", 21, 0, 8192, fibS1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
  {
    printf("S1 task spawn failed\n");
  }
  else
    printf("S1 task spawned\n");

  if(taskSpawn("service-S2", 22, 0, 8192, fibS2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
  {
    printf("S1 task spawn failed\n");
  }
  else
    printf("S1 task spawned\n");

  if(taskSpawn("service-S3", 23, 0, 8192, fibS3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR) 
  {
    printf("S3 task spawn failed\n");
  }
  else
    printf("S3 task spawned\n"); 
   

  /* Simulate the C.I. for S1 and S2 and mark on windview and log
     wvEvent first because F10 and F20 can preempt this task!
   */
  if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
	  printf("WV EVENT ERROR\n");
  semGive(semS1); semGive(semS2); semGive(semS3);


  /* Sequencing loop for LCM phasing of S1, S2, S3
   */
  while(!abortTest)
  {

	  /* Basic sequence of releases after CI */
      taskDelay(3); semGive(semS1);                 /* +3  */
      taskDelay(2); semGive(semS2);                 /* +5  */
      taskDelay(1); semGive(semS1);                 /* +6  */
      taskDelay(2); semGive(semS1);                 /* +8  */
      taskDelay(2); semGive(semS1); semGive(semS2); /* +10 */
      taskDelay(2); semGive(semS1);                 /* +12 */
      taskDelay(2); semGive(semS1);                 /* +14 */
      taskDelay(1);                                 /* +15 */

	  /* back to C.I. conditions, log event first due to preemption */
	  if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
		  printf("WV EVENT ERROR\n");
	  semGive(semS1); semGive(semS2); semGive(semS3);
  }  
 

}

void start(void)
{
	abortTest=0;

	if(taskSpawn("Sequencer", 20, 0, 8192, Sequencer, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
	{
	  printf("Sequencer task spawn failed\n");
	}
	else
	  printf("Sequencer task spawned\n");

}
                                                                  

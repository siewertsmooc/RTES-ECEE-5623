#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "errnoLib.h"

#define HIGH_PRIO_SERVICE 1
#define MIDDLE_PRIO_SERVICE 2
#define LOW_PRIO_SERVICE 3

SEM_ID   msgSem;

int CScount = 0, DoneCount = 0, clkRate, lowStart, highStart;
int runInterference=0;

LOCAL void idle (int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
  while(runInterference);
  exit(0);
}


LOCAL void semPrinter (int id, int msgs, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
  int i, Lock;

  if(id == LOW_PRIO_SERVICE)
    printf("Low prio task requesting shared resource at %d ticks\n", (lowStart=tickGet()));
  else
    printf("High prio task requesting shared resource at %d ticks\n", (highStart=tickGet()));

  /* Take semaphore and wait for chance to print */
  if (semTake (msgSem, WAIT_FOREVER) != OK) 
  {
    printf ("Error taking semaphore in task %d \n", id);
    exit(-1);
  }

  CScount++;

  if(id == LOW_PRIO_SERVICE) 
  {
    printf("Low prio task in CS\n");
    fflush(stdout);
  }

  else {
    printf("High prio task in CS\n");
    fflush(stdout);
  }

  /* camp out in CS long enough to get stuck here */
  taskDelay(clkRate);

  for (i=0; (i<msgs); i++)
  {
    printf ("\nPrint msg # %d from task %d", i, id);
    fflush(stdout);

    if(id == LOW_PRIO_SERVICE) 
    {
      printf(" low prio task\n");
      fflush(stdout);
    }
    else 
    {
      printf(" high prio task\n");
      fflush(stdout);
    }
  }


  if (semGive (msgSem) != OK) 
  {
    printf ("Error giving semaphore in task %d \n", id);
    exit(-1);
  }

  if(id == LOW_PRIO_SERVICE)
    printf("Low prio task done in %d ticks\n", (tickGet()-lowStart));
  else
    printf("High prio task done in %d ticks\n", (tickGet()-highStart));

  DoneCount++;

  exit(0);

}


void prio_invert (int intfCount, int inversionSafe)
{
  int      p1Task, p2Task, p3Task, i;


  CScount=0; DoneCount=0;
  clkRate=sysClkRateGet();

  printf("%d ticks per second\n", clkRate);

  /* Create print to screen semaphore */
  if(inversionSafe)
  {
    if ((msgSem = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE)) == NULL) 
    {
      printf ("Error creating print to screen semaphore\n");
      exit(-1);
    }
    printf ("\nInversion Safe:Print to screen critical section semaphore created\n");
    fflush(stdout);
  }

  else
  {
    if ((msgSem = semMCreate(SEM_Q_PRIORITY)) == NULL)
    {
      printf ("Error creating print to screen semaphore\n");
      exit(-1);
    }
    printf ("\nUnsafe:Print to screen critical section semaphore created\n");
  }

  /* Spawn task to print message to the screen */
  if ((p3Task = taskSpawn ("lowprio_printer", 100, 0, (1024*8), (FUNCPTR) semPrinter, LOW_PRIO_SERVICE,1,0,0,0,0,0,0,0,0)) == ERROR)
  {
    printf ("Error creating lowprio_printer task\n");
    exit(-1);
  }
  printf ("Low priority print to screen task spawned\n");

  /* spin wait while T3 has not yet entered critical section */
  while(CScount < 1) taskDelay(clkRate/10);

  /* Spawn task to print message to the screen */
  if ((p1Task = taskSpawn ("highprio_printer", 50, 0, (1024*8), (FUNCPTR) semPrinter, HIGH_PRIO_SERVICE,1,0,0,0,0,0,0,0,0)) == ERROR)
  {
    printf ("Error creating highprio_printer task\n");
    exit(-1);
  }
  printf ("High priority print to screen task spawned\n");

  /* Spawn an interfering task */
  runInterference=1;
  if ((p2Task = taskSpawn ("interference", 75, 0, (1024*8), (FUNCPTR) idle, MIDDLE_PRIO_SERVICE,0,0,0,0,0,0,0,0,0)) == ERROR)
  {
    printf ("Error creating idle interference task\n");
    exit(-1);
  }
  printf ("Medium priority interference task spawned\n");

  /* interference duration */
  for(i=0;i<intfCount;i++) taskDelay(clkRate);

  /* stop the interference to let low prio finish */
  runInterference=0;
  printf ("Stopped interference\n");

  /* allow the delayed task to finish up */
  while(DoneCount < 2) taskDelay(clkRate/10);

  printf ("\nAll done\n");

  /* shutdown the system resources */
  semDelete (msgSem);

  exit(0);
}


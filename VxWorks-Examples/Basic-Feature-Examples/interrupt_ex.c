#include "semLib.h"
#include "signal.h"
#include "stdlib.h"
#include "stdio.h"   
#include "taskLib.h"   

#include "intLib.h"
#include "iv.h"    
#include "pc.h"    
#include "config.h"    

int global_continue_service = 0;
int global_count = 0;
SEM_ID global_sem;



void release_event(void);


void connect_to_int(void)
{
  FUNCPTR release_event_isr;

  release_event_isr = intHandlerCreate((FUNCPTR)release_event,0);

  intVecSet((FUNCPTR *)INUM_TO_IVEC(PIT0_INT_VEC), (FUNCPTR)release_event_isr);
}


void periodic_release(void)
{

  while(global_continue_service==1)
  {
    release_event();
    taskDelay(1000);
  }

}


void release_event(void)
{
  semGive(global_sem);
}


void event_released_task(void)
{  

  global_continue_service = 1;
  global_count = 0;
  global_sem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

  while(global_continue_service == 1) 
  {
    semTake(global_sem, WAIT_FOREVER);
    global_count++;
  }

  semDelete(global_sem);

}

void start_event_release_demo(void)
{

  if(taskSpawn("tRelDemo", 10, 0, (4*1024), (FUNCPTR)event_released_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
== ERROR) {
    printf("Release task spawn failed\n");
  }
  else
    printf("Release task spawned\n");
}

